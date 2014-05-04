#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include "hash.h"
#include "boundingbox.h"
#include "argparser.h"

class Vertex;
class Face;

// ======================================================================
// ======================================================================
// Stores and renders all the vertices, triangles, and edges for a 3D model

class Mesh {

public:

	// ========================
	// CONSTRUCTOR & DESTRUCTOR
	Mesh(ArgParser *a) { args = a; }
	~Mesh();
	void Load(const std::string &input_file);
	
	// ========
	// VERTICES
	int numVertices() const { return vertices.size(); }
	Vertex* addVertex(const glm::vec3 &pos);
	// look up vertex by index from original .obj file
	Vertex* getVertex(int i) const {
		assert (i >= 0 && i < numVertices());
		Vertex *v = vertices[i];
		assert (v != NULL);
		return v; }

	// =====
	// FACES
	int numFaces() const { return faces.size(); }
	Face* addFace(int num_verts, Vertex** verts);
	Face* addFace(Vertex* p1, Vertex* p2, Vertex* p3);
	Face* addFace(Vertex* p1, Vertex* p2, Vertex* p3, Vertex* p4);
	void removeFace(Face* f);
	void add_adjacency(Face* face);

	// ===============
	// OTHER ACCESSORS
	const BoundingBox& getBoundingBox() const { return bbox; }
	
	// ===+=====
	// RENDERING
	void initializeVBOs();
	void setupVBOs();
	void drawVBOs(const glm::mat4 &ProjectionMatrix,const glm::mat4 &ViewMatrix,const glm::mat4 &ModelMatrix);
	void cleanupVBOs();

	void TriVBOHelper( std::vector<glm::vec3> &indexed_verts,
										 std::vector<unsigned int> &mesh_tri_indices,
										 const glm::vec3 &pos_a,
										 const glm::vec3 &pos_b,
										 const glm::vec3 &pos_c,
										 const glm::vec3 &normal_a,
										 const glm::vec3 &normal_b,
										 const glm::vec3 &normal_c,
										 const glm::vec3 &color_ab,
										 const glm::vec3 &color_bc,
										 const glm::vec3 &color_ca,
										 const glm::vec3 &center_color);

	//Quality
	double calculateMeshQuality();
	void showFaceTypes();
	void refine_mesh_delaunay();
	bool delaunay(Face* face1, Face* face2); //returns whether or not the diagonal was swapped
	bool makes_convex_quad(Face* face1, Face* face2);

private:

	// don't use these constructors
	Mesh(const Mesh &/*m*/) { assert(0); exit(0); }
	const Mesh& operator=(const Mesh &/*m*/) { assert(0); exit(0); }

	 
	// ==============
	// REPRESENTATION
	ArgParser *args;
	std::vector<Vertex*> vertices;
	faceshashtype faces; //I ADDED THIS
	BoundingBox bbox;
	vphashtype vertex_parents;
	int num_mini_triangles;

	GLuint mesh_VAO;
	GLuint mesh_tri_verts_VBO;
	GLuint mesh_tri_indices_VBO;
};

// ======================================================================
// ======================================================================


#endif




