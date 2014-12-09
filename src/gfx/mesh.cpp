#include "mesh.h"
#include "../utils/text.h"
#include "../utils/utils.h"
#include "../includes.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <sys/stat.h>

std::map<std::string, Mesh*> Mesh::sMeshesLoaded;
bool Mesh::use_vram = true;
bool Mesh::use_binary = true;
long Mesh::num_meshes_rendered = 0;
long Mesh::num_triangles_rendered = 0;
bool Mesh::s_initialized = false;

#define FORMAT_ASE 1
#define FORMAT_OBJ 2
#define FORMAT_BIN 3

//define the functions (as global vars)
REGISTER_GLEXT( void, glGenBuffersARB, GLsizei n, GLuint* ids )
REGISTER_GLEXT( void, glBindBufferARB, GLenum target, GLuint id )
REGISTER_GLEXT( void, glBufferDataARB, GLenum target, GLsizei size, const void* data, GLenum usage )
REGISTER_GLEXT( void, glDeleteBuffersARB, GLsizei n, const GLuint* ids )

Mesh* Mesh::Load(const char* filename, bool multimaterial, bool force_load)
{
	assert(filename);

	if (!force_load)
	{
		std::map<std::string, Mesh*>::iterator it = sMeshesLoaded.find(filename);
		if (it != sMeshesLoaded.end())
			return it->second;
	}

	Mesh* m = new Mesh();
	m->name = filename;

	char file_format = 0;
	std::string ext = m->name.substr( m->name.size() - 4,4 );

	if (ext == ".ase" || ext == ".ASE")
		file_format = FORMAT_ASE;
	else if (ext == ".obj" || ext == ".OBJ" )
		file_format = FORMAT_OBJ;
	else if (ext == ".bin" || ext == ".BIN" )
		file_format = FORMAT_BIN;
	else
	{
		std::cerr << "Unknown mesh format: " << filename << std::endl;
		return NULL;
	}
	

	long time = getTime();
	std::cout << "Mesh loading: " << filename << " ... ";
	std::string binfilename = filename;

	if (file_format != FORMAT_BIN)
		binfilename = binfilename + ".bin";

	if (!force_load && use_binary && m->readBin(binfilename.c_str() ))
	{
		std::cout << "[OK BIN]  Faces: " << m->vertices.size() / 3 << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;
		sMeshesLoaded[filename] = m;
		return m;
	}
	
	bool loaded = false;
	if (file_format == FORMAT_OBJ)
		loaded = m->loadOBJ(filename,multimaterial);
	else if (file_format == FORMAT_ASE)
		loaded = m->loadASE(filename,multimaterial);

	if (loaded)
	{
		std::cout << "[OK]  Faces: " << m->vertices.size() / 3 << " Time: " << (getTime() - time) * 0.001 << "sec" << std::endl;
		if (use_binary)
		{
			std::cout << "Writing Bin... ";
			m->writeBin(filename);
			std::cout << "[OK]" << std::endl;
		}
		sMeshesLoaded[filename] = m;
		return m;
	}
	else
	{
		delete m;
		std::cout << "[ERROR]: Mesh not found" << std::endl;
		return NULL;
	}
}

Mesh::Mesh()
{
	//get extensions for VBO
    if(!s_initialized)
	{
		s_initialized = true;

		IMPORT_GLEXT( glGenBuffersARB );
		IMPORT_GLEXT( glBindBufferARB );
		IMPORT_GLEXT( glBufferDataARB );
		IMPORT_GLEXT( glDeleteBuffersARB );
	}


	radius = 0;
	vertices_vbo_id = texcoords_vbo_id = normals_vbo_id = colors_vbo_id = 0;

	primitive = GL_TRIANGLES;
	#ifndef SKIP_COLDET
		collision_model = NULL;
	#endif
	clear();
}

Mesh::~Mesh()
{
	clear();
}


void Mesh::clear()
{
	//Free VBOs
	if (vertices_vbo_id) 
		glDeleteBuffersARB(1,&vertices_vbo_id);
	if (texcoords_vbo_id) 
		glDeleteBuffersARB(1,&texcoords_vbo_id);
	if (normals_vbo_id) 
		glDeleteBuffersARB(1,&normals_vbo_id);
	if (colors_vbo_id) 
		glDeleteBuffersARB(1,&colors_vbo_id);

	//VBOs ids
	vertices_vbo_id = texcoords_vbo_id = normals_vbo_id = colors_vbo_id = 0;

	//buffers
	vertices.clear();
	normals.clear();
	uvs.clear();
	colors.clear();
	for (size_t i = 0; i < calllist_id.size(); i++)
		if (calllist_id[i] != 0)
			glDeleteLists( calllist_id[i], 1 );
	calllist_id.clear();

	#ifndef SKIP_COLDET
		if (collision_model)
			delete collision_model;
	#endif
}

#ifndef SKIP_COLDET
void Mesh::createCollisionModel()
{
	collision_model = newCollisionModel3D();
	//collision_model->setTriangleNumber(vStrip.size() / 3);
	for (size_t count = 0; count < vertices.size()/3; count++)
		collision_model->addTriangle( vertices[count*3].x, vertices[count*3].y, vertices[count*3].z,
									vertices[count*3+1].x, vertices[count*3+1].y, vertices[count*3+1].z,
									vertices[count*3+2].x, vertices[count*3+2].y, vertices[count*3+2].z);

	collision_model->finalize();
}

bool Mesh::testRayCollision(Matrix44 model, Vector3 start, Vector3 front, Vector3& collision, Vector3& normal)
{
	collision_model->setTransform( model.m );
	if (collision_model->rayCollision( start.v , front.v, true) == false)
		return false;

	collision_model->getCollisionPoint( collision.v, true );

	float t1[9],t2[9];
	collision_model->getCollidingTriangles(t1,t2,false);

	Vector3 v1;
	Vector3 v2;
	v1=Vector3(t1[3]-t1[0],t1[4]-t1[1],t1[5]-t1[2]);
	v2=Vector3(t1[6]-t1[0],t1[7]-t1[1],t1[8]-t1[2]);
	v1.normalize();
	v2.normalize();
	normal = v1.cross(v2);

	return true;
}
#endif

typedef struct 
{
	int size;
	Vector3 aabb_min;
	Vector3	aabb_max;
	Vector3	center;
	Vector3	halfsize;
	float radius;
	int material_range[4];
	char streams[4]; //Normal|Uvs|Color|Extra
} sMeshInfo;

bool Mesh::readBin(const char* filename)
{
	FILE *f;
	assert(filename);

	struct stat stbuffer;

	stat(filename,&stbuffer);
	f = fopen(filename,"rb");
	if (f == NULL) return false;

	unsigned int size = stbuffer.st_size;
	char* data = new char[size];
	fread(data,size,1,f);
	fclose(f);

	//watermark
	if ( memcmp(data,"MBIN",4) != 0 )
	{
		std::cout << "Error in Mesh Bin loader, Wrong content: " << filename << std::endl;
		return false;
	}

	char* pos = data + 4;
	sMeshInfo info;
	memcpy(&info,pos,sizeof(sMeshInfo));
	pos += sizeof(sMeshInfo);

	vertices.resize(info.size);
	memcpy((void*)&vertices[0],pos,sizeof(Vector3) * info.size);
	pos += sizeof(Vector3) * info.size;

	if (info.streams[0] == 'N')
	{
		normals.resize(info.size);
		memcpy((void*)&normals[0],pos,sizeof(Vector3) * info.size);
		pos += sizeof(Vector3) * info.size;
	}

	if (info.streams[1] == 'U')
	{
		uvs.resize(info.size);
		memcpy((void*)&uvs[0],pos,sizeof(Vector2) * info.size);
		pos += sizeof(Vector2) * info.size;
	}

	if (info.streams[2] == 'C')
	{
		colors.resize(info.size);
		memcpy((void*)&colors[0],pos,sizeof(Vector4) * info.size);
		pos += sizeof(Vector4) * info.size;
	}

	aabb_max = info.aabb_max;
	aabb_min = info.aabb_min;
	center = info.center;
	halfsize = info.halfsize;
	radius = info.radius;

	for (int i = 0; i < 4; i++)
		if (info.material_range[i] != -1)
			material_range.push_back( info.material_range[i] );
		else
			break;

	delete[] data;

	#ifndef SKIP_COLDET
		createCollisionModel();
	#endif

	return true;
}

bool Mesh::writeBin(const char* filename)
{
	assert(vertices.size());
	std::string s_filename = filename;
	s_filename += ".bin";

	FILE* f = fopen(s_filename.c_str(),"wb");
	if (f == NULL)
	{
		std::cout << "Error writing meshbin: " << s_filename.c_str() << std::endl;
		return false;
	}

	//watermark
	fwrite("MBIN",sizeof(char),4,f);

	sMeshInfo info;
	info.size = vertices.size();
	info.aabb_max = aabb_max;
	info.aabb_min = aabb_min;
	info.center = center;
	info.halfsize = halfsize;
	info.radius = radius;

	info.streams[0] = normals.size() ? 'N' : ' ';
	info.streams[1] = uvs.size() ? 'U' : ' ';
	info.streams[2] = colors.size() ? 'C' : ' ';
	info.streams[3] = ' ';

	for (size_t i = 0; i < 4; i++)
		info.material_range[i] = material_range.size() > i ? material_range[i] : -1;

	//write info
	fwrite((void*)&info, sizeof(sMeshInfo),1, f);

	//write streams
	fwrite((void*)&vertices[0], vertices.size() * sizeof(Vector3), 1,f);
	if (normals.size())
		fwrite((void*)&normals[0],normals.size() * sizeof(Vector3), 1,f);
	if (uvs.size())
		fwrite((void*)&uvs[0], uvs.size() * sizeof(Vector2), 1,f);
	if (colors.size())
		fwrite((void*)&colors[0], colors.size() * sizeof(Vector4), 1,f);

	fclose(f);
	return false;
}

bool Mesh::loadASE(const char* filename, bool multimaterial)
{
	int nVtx,nFcs;
	int count;
	int vId,aId,bId,cId;
	float vtxX,vtxY,vtxZ;
	float nX,nY,nZ;
	text t;
	if (t.create(filename) == false)
		return false;

	/*
	if (multimaterial)
	{
		t.seek ("*NUMSUBMTLS");
		if (t.eof())
			t.reset();
		else
		{
			int num_subm = t.getint();
			for (int imat = 0; imat < num_subm; imat++)
			{
				t.seek ("*MAP_DIFFUSE");
				t.seek ("*BITMAP");
				std::string mat_name( t.getword() );
				mat_name = mat_name.substr(1, mat_name.size() - 2);
				material_name.push_back( mat_name );
				std::cout << "MATERIAL TEXTURE: " << mat_name << std::endl;
			}
		}
	}
	//*/

	t.seek("*MESH_NUMVERTEX");
	nVtx = t.getint();
	t.seek("*MESH_NUMFACES");
	nFcs = t.getint();

	normals.resize(nFcs*3);
	vertices.resize(nFcs*3);
	uvs.resize(nFcs*3);

	std::vector<Vector3> unique_vertices;
	unique_vertices.resize(nVtx);

	const float max_float = 10000000;
	const float min_float = -10000000;
	aabb_min.set(max_float,max_float,max_float);
	aabb_max.set(min_float,min_float,min_float);

	//load unique vertices
	for(count=0;count<nVtx;count++)
	{
		t.seek("*MESH_VERTEX");
		vId = t.getint();
		vtxX=t.getfloat();
		vtxY=t.getfloat();
		vtxZ=t.getfloat();
		Vector3 v(-vtxX,vtxZ,vtxY);
		unique_vertices[count] = v;
		aabb_min.setMin( v );
		aabb_max.setMax( v );
	}
	center = (aabb_max + aabb_min) * 0.5;
	halfsize = (aabb_max - center) * 2;
	radius = max( aabb_max.length(), aabb_min.length() );
	//if (aabb_min.length() > radius) radius = aabb_min.length();
	
	int prev_mat = 0;

	//load faces
	for(count=0;count<nFcs;count++)
	{
		t.seek("*MESH_FACE");
		t.seek("A:");
		aId = t.getint();
		t.seek("B:");
		bId = t.getint();
		t.seek("C:");
		cId = t.getint();
		vertices[count*3 + 0] = unique_vertices[aId];
		vertices[count*3 + 1] = unique_vertices[bId];
		vertices[count*3 + 2] = unique_vertices[cId];

		if (multimaterial)
		{
			t.seek("*MESH_MTLID");
			int current_mat = t.getint();
			if (current_mat != prev_mat)
			{
				material_range.push_back( count );
				prev_mat = current_mat;
			}
		}
	}

	material_range.push_back(nFcs);

	//uvs
	//t.seek("*MESH_MAPPINGCHANNEL");
	//t.seek("*MESH_MAPPINGCHANNEL");

	t.seek("*MESH_NUMTVERTEX");
	nVtx = t.getint();
	std::vector<Vector2> unique_uvs;
	unique_uvs.resize(nVtx);

	for(count=0;count<nVtx;count++)
	{
		t.seek("*MESH_TVERT");
		vId = t.getint();
		vtxX=t.getfloat();
		vtxY=t.getfloat();
		unique_uvs[count]=Vector2(vtxX,vtxY);
	}

	t.seek("*MESH_NUMTVFACES");
	nFcs = t.getint();
	for(count=0;count<nFcs;count++)
	{
		t.seek("*MESH_TFACE");
		t.getint(); //num face
		uvs[count*3] = unique_uvs[ t.getint() ];
		uvs[count*3+1] = unique_uvs[ t.getint() ];
		uvs[count*3+2] = unique_uvs[ t.getint() ];
	}

	//normals
	for(count=0;count<nFcs;count++)
	{
		t.seek("*MESH_VERTEXNORMAL");
		aId = t.getint();
		nX = (float)t.getfloat();
		nY = (float)t.getfloat();
		nZ = (float)t.getfloat();
		normals[count*3]=Vector3(-nX,nZ,nY);
		t.seek("*MESH_VERTEXNORMAL");
		aId = t.getint();
		nX = (float)t.getfloat();
		nY = (float)t.getfloat();
		nZ = (float)t.getfloat();
		normals[count*3+1]=Vector3(-nX,nZ,nY);
		t.seek("*MESH_VERTEXNORMAL");
		aId = t.getint();
		nX = (float)t.getfloat();
		nY = (float)t.getfloat();
		nZ = (float)t.getfloat();
		normals[count*3+2]=Vector3(-nX,nZ,nY);
	}

	#ifndef SKIP_COLDET
		createCollisionModel();
	#endif

	return true;
}

bool Mesh::loadOBJ(const char* filename, bool multimaterial)
{
	struct stat stbuffer;

	FILE* f = fopen(filename,"rb");
	if (f == NULL)
	{
		std::cerr << "File not found: " << filename << std::endl;
		return false;
	}

	stat(filename,&stbuffer);

	unsigned int size = stbuffer.st_size;
	char* data = new char[size+1];
	fread(data,size,1,f);
	fclose(f);
	data[size] = 0;

	char* pos = data;
	char line[255];
	int i = 0;

	std::vector<Vector3> indexed_positions;
	std::vector<Vector3> indexed_normals;
	std::vector<Vector2> indexed_uvs;

	const float max_float = 10000000;
	const float min_float = -10000000;
	aabb_min.set(max_float,max_float,max_float);
	aabb_max.set(min_float,min_float,min_float);

	unsigned int vertex_i = 0;

	//parse file
	while(*pos != 0)
	{
		if (*pos == '\n') pos++;
		if (*pos == '\r') pos++;

		//read one line
		i = 0;
		while(i < 255 && pos[i] != '\n' && pos[i] != '\r' && pos[i] != 0) i++;
		memcpy(line,pos,i);
		line[i] = 0;
		pos = pos + i;

		//std::cout << "Line: \"" << line << "\"" << std::endl;
		if (*line == '#' || *line == 0) continue; //comment

		//tokenize line
		std::vector<std::string> tokens = tokenize(line," ");

		if (tokens.empty()) continue;

		if (tokens[0] == "v" && tokens.size() == 4)
		{
			Vector3 v( (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) );
			indexed_positions.push_back(v);

			aabb_min.setMin( v );
			aabb_max.setMax( v );
		}
		else if (tokens[0] == "vt" && tokens.size() == 4)
		{
			Vector2 v( (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()) );
			indexed_uvs.push_back(v);
		}
		else if (tokens[0] == "vn" && tokens.size() == 4)
		{
			Vector3 v( (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) );
			indexed_normals.push_back(v);
		}
		else if (tokens[0] == "s") //surface? it appears one time before the faces
		{
			//process mesh
			if (uvs.size() == 0 && indexed_uvs.size() )
				uvs.resize(1);
		}
		else if (tokens[0] == "f" && tokens.size() >= 4)
		{
			Vector3 v1,v2,v3;
			v1.parseFromText( tokens[1].c_str(), '/' );

			for (size_t iPoly = 2; iPoly < tokens.size() - 1; iPoly++)
			{
				v2.parseFromText( tokens[iPoly].c_str(), '/' );
				v3.parseFromText( tokens[iPoly+1].c_str(), '/' );

				vertices.push_back( indexed_positions[ unsigned int(v1.x) -1 ] );
				vertices.push_back( indexed_positions[ unsigned int(v2.x) -1] );
				vertices.push_back( indexed_positions[ unsigned int(v3.x) -1] );
				//triangles.push_back( VECTOR_INDICES_TYPE(vertex_i, vertex_i+1, vertex_i+2) ); //not needed
				vertex_i += 3;

				if (indexed_uvs.size() > 0)
				{
					uvs.push_back( indexed_uvs[ unsigned int(v1.y) -1] );
					uvs.push_back( indexed_uvs[ unsigned int(v2.y) -1] );
					uvs.push_back( indexed_uvs[ unsigned int(v3.y) -1] );
				}

				if (indexed_normals.size() > 0)
				{
					normals.push_back( indexed_normals[ unsigned int(v1.z) -1] );
					normals.push_back( indexed_normals[ unsigned int(v2.z) -1] );
					normals.push_back( indexed_normals[ unsigned int(v3.z) -1] );
				}
			}
		}
	}



	center = (aabb_max + aabb_min) * 0.5;
	halfsize = (aabb_max - center) * 2;
	radius = max( aabb_max.length(), aabb_min.length() );

	material_range.push_back(vertices.size() / 3.0);

	#ifndef SKIP_COLDET
		createCollisionModel();
	#endif
	return true;
}


void Mesh::render(unsigned int submesh_id, bool ignore_vram )
{
	//int count;
	assert(vertices.size() && "No vertices in this mesh");

	bool creating_calllist = false;

	num_meshes_rendered++;
	num_triangles_rendered += vertices.size() / 3;

	//first time rendering
	/*
	if ((use_vram && !ignore_vram ) && calllist_id.empty())
	{
		calllist_id.resize( material_range.size() );
		for (int i = 0; i < calllist_id.size(); i++)
			calllist_id[i] = 0;
	}

	if (!ignore_vram)
	{	
		//create
		if (calllist_id[ submesh_id ] == 0)
		{
			calllist_id[ submesh_id ] = glGenLists(1);
			glNewList(calllist_id[ submesh_id ], GL_COMPILE );
			creating_calllist = true;
		}  
		else //use
		{
			glCallList( calllist_id[ submesh_id ] );
			return;
		}
	}
	*/

	//use vbos
	if (use_vram && !ignore_vram) //vertex buffer objects
	{
		if (vertices_vbo_id == 0)
			uploadToVRAM();

		glEnableClientState(GL_VERTEX_ARRAY);
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, vertices_vbo_id );
		glVertexPointer( 3, GL_FLOAT, 0, (char *) NULL );

		if (normals.size())
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, normals_vbo_id );
			glNormalPointer(GL_FLOAT, 0, (char *) NULL );
		}

		if (uvs.size())
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, texcoords_vbo_id );
			glTexCoordPointer( 2, GL_FLOAT, 0, (char *) NULL );
		}

		if (colors.size())
		{
			glEnableClientState(GL_COLOR_ARRAY );
			glBindBufferARB( GL_ARRAY_BUFFER_ARB, colors_vbo_id );
			glColorPointer(4,GL_FLOAT, 0, &colors[0] );
		}
	}
	else //vertex arrays
	{
		if (normals.size())
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, &normals[0] );
		}
		
		if (uvs.size())
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2,GL_FLOAT, 0, &uvs[0] );
		}

		if (colors.size())
		{
			glEnableClientState(GL_COLOR_ARRAY );
			glColorPointer(4,GL_FLOAT, 0, &colors[0] );
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, &vertices[0] );
	}

	int start = 0;
	if (submesh_id > 0) 
		material_range[submesh_id-1] *= 3;
	int size = vertices.size();
	if (!material_range.empty())
		size = material_range[submesh_id]*3 - start;

	glDrawArrays(primitive, start, size);
	//glDrawArrays(primitive, 0, vertices.size() ); //all

	if (vertices_vbo_id)
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

	glDisableClientState(GL_VERTEX_ARRAY);

	if (normals.size())
		glDisableClientState(GL_NORMAL_ARRAY);
	if (uvs.size())
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (colors.size())
		glDisableClientState(GL_COLOR_ARRAY);

	if (creating_calllist)
	{
		glEndList();
		glCallList( calllist_id[ submesh_id ] );
	}
}

void Mesh::uploadToVRAM()
{
	assert(vertices.size() && vertices_vbo_id == 0);
	if (glGenBuffersARB == 0)
	{
		std::cout << "Error: your graphics cards dont support VBOs. Sorry." << std::endl;
		exit(0);
	}

	// Vertices
	glGenBuffersARB( 1, &vertices_vbo_id );
	glBindBufferARB( GL_ARRAY_BUFFER_ARB, vertices_vbo_id );
	glBufferDataARB( GL_ARRAY_BUFFER_ARB, vertices.size()*3*sizeof(float), &vertices[0], GL_STATIC_DRAW_ARB );

	// UVs
	if (uvs.size())
	{
		glGenBuffersARB( 1, &texcoords_vbo_id );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, texcoords_vbo_id );
		glBufferDataARB( GL_ARRAY_BUFFER_ARB, uvs.size()*2*sizeof(float), &uvs[0], GL_STATIC_DRAW_ARB );
	}

	// Normals
	if (normals.size())
	{
		glGenBuffersARB( 1, &normals_vbo_id );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, normals_vbo_id );
		glBufferDataARB( GL_ARRAY_BUFFER_ARB, normals.size()*3*sizeof(float), &normals[0], GL_STATIC_DRAW_ARB );
	}

	// Colors
	if (colors.size())
	{
		glGenBuffersARB( 1, &colors_vbo_id );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, colors_vbo_id );
		glBufferDataARB( GL_ARRAY_BUFFER_ARB, colors.size()*4*sizeof(float), &colors[0], GL_STATIC_DRAW_ARB );
	}

	glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

	checkGLErrors();

	//clear buffers to save memory
}

void Mesh::renderDebug()
{
	size_t count;
	glPointSize(3.0f);
	glColor3f(1.0f,0.0f,0.0f);

	glBegin(GL_POINTS);
	for(count=0;count<vertices.size();count++)
		glVertex3f(vertices[count].x,vertices[count].y,vertices[count].z);
	glEnd();

	glColor3f(0.3f,0.3f,0.3f);
	glBegin(GL_TRIANGLES);
		for(count=0;count<vertices.size();count++)
			glVertex3f(vertices[count].x,vertices[count].y,vertices[count].z);
	glEnd();

	glColor3f(0.0f,1.0f,0.0f);
	glBegin(GL_LINES);
	for(count=0;count<normals.size();count++)
	{
		glVertex3fv( vertices[count].v );
		glVertex3fv( (vertices[count] + normals[count] * 3).v );
	}
	glEnd();
}

void Mesh::renderAABB()
{
	glPushMatrix();
		glTranslatef( center.x, center.y, center.z);
		glScalef( halfsize.x, halfsize.y, halfsize.z );
#ifdef USE_GLUT
		glutWireCube(1);
#endif
	glPopMatrix();
}

void Mesh::createSolidBox(float sizex, float sizey, float sizez)
{
	float vert[] = { -1,1,-1,-1,-1,+1,-1,1,1,-1,1,-1,-1,-1,-1,-1,-1,+1,1,1,-1,1,1,1,1,-1,+1,1,1,-1,1,-1,+1,1,-1,-1,-1,1,1,1,-1,1,1,1,1,-1,1,1,-1,-1,1,1,-1,1,-1,1,-1,1,1,-1,1,-1,-1,-1,1,-1,1,-1,-1,-1,-1,-1,-1,1,-1,1,1,1,1,1,-1,-1,1,-1,-1,1,1,1,1,1,-1,-1,-1,1,-1,-1,1,-1,1,-1,-1,-1,1,-1,1,-1,-1,1 };
	float norm[] = { -1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0 };
	float coord[] = { 0,1, 1,0 ,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,0,1,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0 };

	vertices.resize( 108 / 3);
	normals.resize( 108 / 3);
	uvs.resize( 72 / 2);

	memcpy( &vertices[0], vert, sizeof(Vector3) * vertices.size() );
	memcpy( &normals[0], norm, sizeof(Vector3) * normals.size() );
	memcpy( &uvs[0], coord, sizeof(Vector2) * uvs.size() );

	Vector3 size(sizex*0.5f, sizey*0.5f, sizez*0.5f);

	for(int i = 0; i < vertices.size(); i++)
		vertices[i] *= size;

	material_range.push_back(vertices.size()/3);
}

void Mesh::createWireBox(float sizex, float sizey, float sizez)
{
}
