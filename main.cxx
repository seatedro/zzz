#include <cmath>
#include <cstdio>
#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

struct ChunkPos {
    int x, z;
    bool operator==(const ChunkPos& o) const
    {
        return x == o.x && z == o.z;
    }
};
// it is entirely too restarted that i need to write the hash function like this using an overload
struct ChunkHash {
    size_t operator()(const ChunkPos& p) const
    {
        return p.x * 31 + p.z;
    }
};

struct Chunk {
    Model model;
    double avgHeight;
};

struct PerlinNoise {
    std::vector<int> p;

    PerlinNoise(unsigned int seed)
    {
        p.resize(512);
        std::iota(p.begin(), p.begin() + 256, 0);
        std::copy(p.begin(), p.begin() + 256, p.begin() + 256);
        std::shuffle(p.begin(), p.begin() + 256, std::default_random_engine(seed));
    }

    double noise(double x, double y, double z)
    {
        int X = (int)floor(x) & 255, Y = (int)floor(y) & 255, Z = (int)floor(z) & 255;
        x -= floor(x);
        y -= floor(y);
        z -= floor(z);
        double u = fade(x), v = fade(y), w = fade(z);
        int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
        int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;
        return lerp(w,
            lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
                lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
            lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
                lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
    }

    float fbm(float x, float y, int octaves = 6, float persistence = 0.5f, float lacunarity = 2.0f,
        float scale = 200.0f)
    {
        float value = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;

        for (int i = 0; i < octaves; i++) {
            float nx = x * frequency / scale;
            float ny = y * frequency / scale;
            // float nz = i * 2.0f;

            float n = noise(nx, ny, 0.0f);
            value += n * amplitude;

            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return std::pow(value, 1.0);
    }

    double fade(double t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    double lerp(double t, double a, double b)
    {
        return a + t * (b - a);
    }
    double grad(int hash, double x, double y, double z)
    {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x
                                                  : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};

Color getTerrainColor(float height)
{
    if (height < 0.2f)
        return DARKBLUE; // Deep water
    else if (height < 0.35f)
        return BLUE; // Shallow water
    else if (height < 0.4f)
        return SKYBLUE; // Coastal areas
    else if (height < 0.45f)
        return BEIGE; // Sand/beaches
    else if (height < 0.475f)
        return LIME; // Lowlands (lighter green)
    else if (height < 0.5f)
        return GREEN; // Mid-elevation grass
    else if (height < 0.525f)
        return DARKGREEN; // Higher grass/forests
    else if (height < 0.65f)
        return BROWN; // Lower mountains
    else if (height < 0.7f)
        return DARKBROWN; // Mid mountains
    else if (height < 0.75f)
        return GRAY; // High mountains
    else
        return WHITE; // Snow peaks
}

const int CHUNK_SIZE = 16;
const int RENDER_DISTANCE = 3;
constexpr int RESOLUTION = CHUNK_SIZE + 1;
constexpr int VERT_COUNT = RESOLUTION * RESOLUTION;
// i have no idea what i'm doing
// i chose these randomly after destroying my computer with seg faults
const double HEIGHT_SCALE = 40.0f;
const double NOISE_SCALE = 20.0f;

struct Terrain {
    Mesh mesh;
    double avg_height;
    Terrain()
    {
        mesh = { 0 };
        mesh.vertexCount = VERT_COUNT;
        mesh.triangleCount = (RESOLUTION - 1) * (RESOLUTION - 1) * 2;

        mesh.vertices = new float[VERT_COUNT * 3];
        mesh.normals = new float[VERT_COUNT * 3];
        mesh.texcoords = new float[VERT_COUNT * 2];
        mesh.indices = new unsigned short[mesh.triangleCount * 3];
        mesh.colors = new unsigned char[VERT_COUNT * 4];
    }
    void generate(int chunkX, int chunkZ, PerlinNoise& perlin, float height_scale, float noise_scale, float persistence, float lacunarity)
    {
        double total_normalized_height = 0.0f;

        const float A = 2.0f; // apologies for magic numebr, ~~roughly equal to max of fbm
        for (int z = 0; z < RESOLUTION; z++) {
            for (int x = 0; x < RESOLUTION; x++) {
                int i = z * RESOLUTION + x;
                double world_x = (chunkX * CHUNK_SIZE + x);
                double world_z = (chunkZ * CHUNK_SIZE + z);
                double raw_height = perlin.fbm(world_x * noise_scale, world_z * noise_scale, 4, persistence, lacunarity);
                double normalized_height = (raw_height + A) / (2 * A);
                total_normalized_height += normalized_height;
                double height = normalized_height * height_scale;

                mesh.vertices[i * 3] = x;
                mesh.vertices[i * 3 + 1] = height;
                mesh.vertices[i * 3 + 2] = z;

                mesh.normals[i * 3] = 0.0f;
                mesh.normals[i * 3 + 1] = 1.0f;
                mesh.normals[i * 3 + 2] = 0.0f;

                mesh.texcoords[i * 2] = (float)x / CHUNK_SIZE;
                mesh.texcoords[i * 2 + 1] = (float)z / CHUNK_SIZE;

                Color color = getTerrainColor(normalized_height);
                mesh.colors[i * 4] = color.r;
                mesh.colors[i * 4 + 1] = color.g;
                mesh.colors[i * 4 + 2] = color.b;
                mesh.colors[i * 4 + 3] = color.a;
            }
        }

        avg_height = total_normalized_height / (RESOLUTION * RESOLUTION);

        int idx = 0;
        for (int z = 0; z < RESOLUTION - 1; z++) {
            for (int x = 0; x < RESOLUTION - 1; x++) {
                int tl = z * RESOLUTION + x;
                int tr = tl + 1;
                int bl = (z + 1) * RESOLUTION + x;
                int br = bl + 1;

                mesh.indices[idx++] = tl;
                mesh.indices[idx++] = bl;
                mesh.indices[idx++] = tr;
                mesh.indices[idx++] = tr;
                mesh.indices[idx++] = bl;
                mesh.indices[idx++] = br;
            }
        }

        UploadMesh(&mesh, false);
    }
};

int main()
{
    InitWindow(1280, 720, "zzz");

    Camera3D camera = { (Vector3) { 0.0f, 40.0f, 50.0f }, (Vector3) { 0.0f, 0.0f, 0.0f }, (Vector3) { 0.0f, 1.0f, 0.0f }, 60.0f,
        CAMERA_PERSPECTIVE };

    PerlinNoise noise(42);
    std::unordered_map<ChunkPos, Chunk, ChunkHash> chunks;
    int last_cam_chunk_x = -9999, last_cam_chunk_z = -9999;

    float height_scale = HEIGHT_SCALE;
    float noise_scale = NOISE_SCALE;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    bool regenerate = false;
    bool show_ui = false;

    SetTargetFPS(60);
    DisableCursor();
    SetExitKey(KEY_Q);

    while (!WindowShouldClose()) {
        if (!show_ui) {
            GuiLock();
            UpdateCamera(&camera, CAMERA_FIRST_PERSON);
        }

        int cam_chunk_x = (int)floor(camera.position.x / CHUNK_SIZE);
        int cam_chunk_z = (int)floor(camera.position.z / CHUNK_SIZE);

        if (cam_chunk_x != last_cam_chunk_x || cam_chunk_z != last_cam_chunk_z || regenerate) {
            for (auto& pair : chunks) {
                UnloadModel(pair.second.model);
            }
            chunks.clear();

            // generating new chunks around camera....
            for (int z = cam_chunk_z - RENDER_DISTANCE; z <= cam_chunk_z + RENDER_DISTANCE; z++) {
                for (int x = cam_chunk_x - RENDER_DISTANCE; x <= cam_chunk_x + RENDER_DISTANCE; x++) {
                    ChunkPos pos = { x, z };
                    Terrain terrain = Terrain();
                    terrain.generate(x, z, noise, height_scale, noise_scale, persistence, lacunarity);
                    Model model = LoadModelFromMesh(terrain.mesh);

                    // this shit made everything green....
                    // i'm retaded
                    // i need to paint the vertices
                    // Color chunkColor = getTerrainColor(avgHeight);
                    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
                    chunks[pos] = { model, terrain.avg_height };
                }
            }

            last_cam_chunk_x = cam_chunk_x;
            last_cam_chunk_z = cam_chunk_z;
            regenerate = false;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            show_ui = !show_ui;
        }

        // Draw
        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(camera);

        for (const auto& pair : chunks) {
            DrawModel(pair.second.model,
                (Vector3) { (float)(pair.first.x * CHUNK_SIZE), 0, (float)(pair.first.z * CHUNK_SIZE) }, 1.0f,
                WHITE);
        }

        EndMode3D();

        if (show_ui) {
            GuiUnlock();
            EnableCursor();
            GuiGroupBox({ 10, 10, 250, 200 }, "Terrain Controls");

            if (GuiSlider({ 20, 40, 230, 20 }, "Height Scale", TextFormat("%.1f", height_scale), &height_scale, 5.0f, 50.0f)) {
                regenerate = true;
            }
            if (GuiSlider({ 20, 70, 230, 20 }, "Noise Scale", TextFormat("%.1f", noise_scale), &noise_scale, 5.0f, 50.0f)) {
                regenerate = true;
            }
            if (GuiSlider({ 20, 100, 230, 20 }, "Persistence", TextFormat("%.2f", persistence), &persistence, 0.1f, 1.0f)) {
                regenerate = true;
            }
            if (GuiSlider({ 20, 130, 230, 20 }, "Lacunarity", TextFormat("%.1f", lacunarity), &lacunarity, 1.0f, 4.0f)) {
                regenerate = true;
            }
        }

        EndDrawing();
    }

    for (auto& pair : chunks) {
        UnloadModel(pair.second.model);
    }

    CloseWindow();
    return 0;
}
