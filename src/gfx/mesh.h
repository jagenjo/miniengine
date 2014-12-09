#ifndef MESH_H
#define MESH_H

#include <vector>
#include "../utils/math.h"
#include "../extra/coldet/coldet.h"

#include <map>
#include <string>

class Mesh
{
public:
	static std::map<std::string, Mesh*> Mesh::sMeshesLoaded;
	static bool use_vram;
	static bool use_binary;
	static long num_meshes_rendered;
	static long num_triangles_rendered;
	static bool s_initialized;

	std::string name;

	std::vector<std::string> material_name; 
	std::vector<unsigned int> material_range; 

	std::vector< Vector3 > vertices; //here we store the vertices
	std::vector< Vector3 > normals;	 //here we store the normals
	std::vector< Vector2 > uvs;	 //here we store the texture coordinates
	std::vector< Vector4 > colors; //here we store the colors

	Vector3 aabb_min;
	Vector3	aabb_max;
	Vector3	center;
	Vector3	halfsize;
	float radius;
	unsigned int primitive;

	std::vector<unsigned int> calllist_id;
	unsigned int vertices_vbo_id;
	unsigned int texcoords_vbo_id;
	unsigned int normals_vbo_id;
	unsigned int colors_vbo_id;

	Mesh();
	~Mesh();

	void clear();
	void render(unsigned int submesh_id = 0, bool ignore_vram = false);
	void renderDebug();
	void renderAABB();

	void createWireBox(float sizex, float sizey, float sizez);
	void createSolidBox(float sizex, float sizey, float sizez);

	bool readBin(const char* filename);
	bool writeBin(const char* filename);

	unsigned int getNumSubmaterials() { return material_name.size(); }
	unsigned int getNumSubmeshes() { return material_range.size(); }

	#ifndef SKIP_COLDET
		CollisionModel3D* collision_model;
		void createCollisionModel();
		bool testRayCollision(Matrix44 model, Vector3 start, Vector3 front, Vector3& collision, Vector3& normal);
	#endif

	static Mesh* Load(const char* filename, bool multimaterial = false, bool force_load = false);

private:
	bool loadASE(const char* filename, bool multimaterial = false);
	bool loadOBJ(const char* filename, bool multimaterial = false);
	void uploadToVRAM();
};

#endif