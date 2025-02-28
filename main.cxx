#include <cmath>
#include <cstdio>
#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>

#include "raylib.h"

struct ChunkPos {
  int x, z;
  bool operator==(const ChunkPos& o) const { return x == o.x && z == o.z; }
};
// it is entirely too restarted that i need to write the hash function like this using an overload
struct ChunkHash {
  size_t operator()(const ChunkPos& p) const { return p.x * 31 + p.z; }
};

struct Chunk {
  Model model;
  double avgHeight;
};

struct PerlinNoise {
  std::vector<int> p;

  PerlinNoise(unsigned int seed) {
    p.resize(512);
    std::iota(p.begin(), p.begin() + 256, 0);
    std::copy(p.begin(), p.begin() + 256, p.begin() + 256);
    std::shuffle(p.begin(), p.begin() + 256, std::default_random_engine(seed));
  }

  double noise(double x, double y, double z) {
    int X = (int)floor(x) & 255, Y = (int)floor(y) & 255,
        Z = (int)floor(z) & 255;
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);
    double u = fade(x), v = fade(y), w = fade(z);
    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;
    return lerp(
        w,
        lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
             lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
        lerp(v,
             lerp(u, grad(p[AA + 1], x, y, z - 1),
                  grad(p[BA + 1], x - 1, y, z - 1)),
             lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                  grad(p[BB + 1], x - 1, y - 1, z - 1))));
  }

  float fbm(float x, float y, int octaves = 6, float persistence = 0.5f,
            float lacunarity = 2.0f, float scale = 200.0f) {
      float value = 0.0f;
      float amplitude = 1.0f;
      float frequency = 1.0f;

      for (int i = 0; i < octaves; i++) {
          float nx = x * frequency / scale;
          float ny = y * frequency / scale;
          float nz = i * 10.0f;

          float n = noise(nx, ny, nz);
          value += n * amplitude;

          amplitude *= persistence;
          frequency *= lacunarity;
      }

      return (1.0f / (1.0f + expf(-value * 2.0f))); // [0,1] type shit
  }

  double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
  double lerp(double t, double a, double b) { return a + t * (b - a); }
  double grad(int hash, double x, double y, double z) {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
  }
};

Color getTerrainColor(float height) {
    if (height < 0.25f) {
        return DARKBLUE;      // Deep water
    } else if (height < 0.3f) {
        return BLUE;          // Water
    } else if (height < 0.35f) {
        return SKYBLUE;       // Shallow water
    } else if (height < 0.4f) {
        return BEIGE;         // Sand
    } else if (height < 0.55f) {
        return GREEN;         // Grass
    } else if (height < 0.65f) {
        return DARKGREEN;     // Forest
    } else if (height < 0.75f) {
        return BROWN;         // Mountains
    } else if (height < 0.85f) {
        return DARKBROWN;     // High mountains
    } else if (height < 0.95f) {
        return LIGHTGRAY;     // Stone
    } else {
        return WHITE;         // Snow peaks
    }
}


const int CHUNK_SIZE = 16;
const int RENDER_DISTANCE = 3;
// i have no idea what i'm doing
// i chose these randomly after destroying my computer with seg faults
const double HEIGHT_SCALE = 10.0f;
const double NOISE_SCALE = 80.0f;

std::pair<Mesh, double> generateTerrain(int chunkX, int chunkZ,
                                        PerlinNoise& perlin) {
  int resolution = CHUNK_SIZE + 1;
  int vertCount = resolution * resolution;

  Mesh mesh = {0};
  mesh.vertexCount = vertCount;
  mesh.triangleCount = (resolution - 1) * (resolution - 1) * 2;

  mesh.vertices = (float*)MemAlloc(vertCount * 3 * sizeof(float));
  mesh.normals = (float*)MemAlloc(vertCount * 3 * sizeof(float));
  mesh.texcoords = (float*)MemAlloc(vertCount * 2 * sizeof(float));
  mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 *
                                           sizeof(unsigned short));
  mesh.colors = (unsigned char*)MemAlloc(vertCount * 4 * sizeof(unsigned char));

  double total_normalized_height = 0.0f;

  for (int z = 0; z < resolution; z++) {
    for (int x = 0; x < resolution; x++) {
      int i = z * resolution + x;
      double world_x = (chunkX * CHUNK_SIZE + x);
      double world_z = (chunkZ * CHUNK_SIZE + z);
      double terrain_height =
          perlin.fbm(world_x * NOISE_SCALE, world_z * NOISE_SCALE);
      double normalizedHeight = (terrain_height + 1.0f) * 0.5f;
      total_normalized_height += normalizedHeight;
      double height = terrain_height * HEIGHT_SCALE;

      mesh.vertices[i * 3] = x;
      mesh.vertices[i * 3 + 1] = height;
      mesh.vertices[i * 3 + 2] = z;

      mesh.normals[i * 3] = 0.0f;
      mesh.normals[i * 3 + 1] = 1.0f;
      mesh.normals[i * 3 + 2] = 0.0f;

      mesh.texcoords[i * 2] = (float)x / CHUNK_SIZE;
      mesh.texcoords[i * 2 + 1] = (float)z / CHUNK_SIZE;

      Color color = getTerrainColor(normalizedHeight);
      mesh.colors[i * 4] = color.r;
      mesh.colors[i * 4 + 1] = color.g;
      mesh.colors[i * 4 + 2] = color.b;
      mesh.colors[i * 4 + 3] = color.a;

    }
  }

  double avg_normalized_height =
      total_normalized_height / (resolution * resolution);

  int idx = 0;
  for (int z = 0; z < resolution - 1; z++) {
    for (int x = 0; x < resolution - 1; x++) {
      int tl = z * resolution + x;
      int tr = tl + 1;
      int bl = (z + 1) * resolution + x;
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
  return {mesh, avg_normalized_height};
}


int main() {
  InitWindow(1280, 720, "zzz");

  Camera3D camera = {(Vector3){0.0f, 10.0f, 50.0f}, (Vector3){0.0f, 0.0f, 0.0f},
                     (Vector3){0.0f, 1.0f, 0.0f}, 60.0f, CAMERA_PERSPECTIVE};

  PerlinNoise noise(42);
  std::unordered_map<ChunkPos, Chunk, ChunkHash> chunks;
  int last_cam_chunk_x = -9999, last_cam_chunk_z = -9999;

  SetTargetFPS(60);
  DisableCursor();

  while (!WindowShouldClose()) {
    UpdateCamera(&camera, CAMERA_FIRST_PERSON);

    int cam_chunk_x = (int)floor(camera.position.x / CHUNK_SIZE);
    int cam_chunk_z = (int)floor(camera.position.z / CHUNK_SIZE);

    if (cam_chunk_x != last_cam_chunk_x || cam_chunk_z != last_cam_chunk_z) {
      for (auto& pair : chunks) {
        UnloadModel(pair.second.model);
      }
      chunks.clear();

      // generating new chunks around camera....
      for (int z = cam_chunk_z - RENDER_DISTANCE;
           z <= cam_chunk_z + RENDER_DISTANCE; z++) {
        for (int x = cam_chunk_x - RENDER_DISTANCE;
             x <= cam_chunk_x + RENDER_DISTANCE; x++) {
          ChunkPos pos = {x, z};
          auto [mesh, avgHeight] = generateTerrain(x, z, noise);
          Model model = LoadModelFromMesh(mesh);

          // this shit made everything green....
          // i'm retaded
          // i need to paint the vertices
          // Color chunkColor = getTerrainColor(avgHeight);
          model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
          chunks[pos] = {model, avgHeight};
        }
      }

      last_cam_chunk_x = cam_chunk_x;
      last_cam_chunk_z = cam_chunk_z;
    }

    // Draw
    BeginDrawing();
    ClearBackground(SKYBLUE);
    BeginMode3D(camera);

    for (const auto& pair : chunks) {
      DrawModel(pair.second.model,
                (Vector3){(float)(pair.first.x * CHUNK_SIZE), 0,
                          (float)(pair.first.z * CHUNK_SIZE)},
                1.0f, WHITE);
    }

    EndMode3D();
    DrawFPS(10, 10);

    DrawText("WASD: Move, Mouse: Look", 10, 40, 20, BLACK);
    EndDrawing();
  }

  for (auto& pair : chunks) {
    UnloadModel(pair.second.model);
  }

  CloseWindow();
  return 0;
}
