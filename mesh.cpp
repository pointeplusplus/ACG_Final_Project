#include "glCanvas.h"
#include <fstream>
#include <sstream>
#include <cmath>

#include "mesh.h"
#include "vertex.h"
#include "face.h"

#define ANGLE_THRESHOLD 0.35 //about 20 degrees
//#define ANGLE_THRESHOLD 0.7 //about 40 degrees
//#define ANGLE_THRESHOLD 500 // no threshold

// to give a unique id number to each triangles
int Face::next_face_id = 0;

glm::vec3 black(0.0,0.0,0.0);
glm::vec3 white(1.0,1.0,1.0);

// =======================================================================
// MESH DESTRUCTOR 
// =======================================================================

Mesh::~Mesh() {
	cleanupVBOs();

	// delete all the triangles
	std::vector<Face*> todo;
	for (faceshashtype::iterator iter = faces.begin();
			 iter != faces.end(); iter++) {
		Face* f = iter->second;
		todo.push_back(f);
	}
	int num_faces = todo.size();
	for (int i = 0; i < num_faces; i++) {
		removeFace(todo[i]);
	}
	// delete all the vertices
	int num_vertices = numVertices();
	for (int i = 0; i < num_vertices; i++) {
		delete vertices[i];
	}
}

// =======================================================================
// MODIFIERS:	 ADD & REMOVE
// =======================================================================

Vertex* Mesh::addVertex(const glm::vec3 &position) {
	int index = numVertices();
	Vertex *v = new Vertex(index, position);
	vertices.push_back(v);
	if (numVertices() == 1)
		bbox = BoundingBox(position,position);
	else 
		bbox.Extend(position);
	return v;
}

void Mesh::add_adjacency(Face* face){
//	std::cout << "In mesh's add adjacency func" << std::endl;

	int num_verts_in_common = 0;
	//add ajacency
	for(faceshashtype::iterator f = faces.begin(); f != faces.end(); f++){

		Face* face_to_check = f->second;

		//no self adjacency
		if(face_to_check != face){
//			std::cout << "number of vertices in face " << face->get_num_vertices() << std::endl; 
			num_verts_in_common = 0;
			for(int v = 0; v < face->get_num_vertices(); v++){

				if(face_to_check->has_vertex((*face)[v]) ){
					num_verts_in_common++;
				}
			}
			if(num_verts_in_common == 2){ // need to add the adjacency 
//				std::cout << " TWO IN COMMON " << std::endl;
				face->add_adjacency(face_to_check);
				face_to_check->add_adjacency(face);
			}
			if(num_verts_in_common != 0){
//				std::cout << "num common verts = " << num_verts_in_common << std::endl;
			}
			//this would mean a bad mesh
			//BREAKS FOR BAD MESH 
			//this mesh will break the delauny, but a quality calculation could still be preformed
			//std::cout << "THERE ARE FACES WITH MORE THAN TWO VERTICES IN COMMON" << std::endl;

//			assert (num_verts_in_common < 3);

		}
	}

}

void Mesh::addFace(int num_verts, Vertex** verts) {
	// create the triangle

	Face* f;

	if(num_verts == 3){
		f = new TriangleFace();
	}
	else if(num_verts == 4){
		f = new QuadFace();
	}
	else{
		assert(0);
	}

	f->setVertices(num_verts, verts);
//	std::cout << " made face with " << f->get_num_vertices() << " vertices" << std::endl;

	// add the triangle to the master list
	assert (faces.find(f->getID()) == faces.end());
	faces[f->getID()] = f;
	add_adjacency(f);
	
}

void Mesh::addFace(Vertex* p1, Vertex* p2, Vertex* p3){
	Face* f = new TriangleFace();
	Vertex* verts[3]= {p1, p2, p3};
	f->setVertices(3, verts);
//	std::cout << " made triangle with " << f->get_num_vertices() << " vertices" << std::endl;
	assert (faces.find(f->getID()) == faces.end());
	faces[f->getID()] = f;
	add_adjacency(f);
}
void Mesh::addFace(Vertex* p1, Vertex* p2, Vertex* p3, Vertex* p4){
	Face* f = new QuadFace();
	Vertex* verts[4]= {p1, p2, p3,p4};
	f->setVertices(4, verts);
	assert (faces.find(f->getID()) == faces.end());
	faces[f->getID()] = f;
	add_adjacency(f);
}

void Mesh::removeFace(Face *f) {



	//get rid of adjacencies to the face being removed
	//just go through all of them and make sure it's not still on their lists
	for(faceshashtype::iterator curr_f = faces.begin(); curr_f != faces.end(); curr_f++){

		Face* current_face = curr_f->second;

		current_face->remove_adjacency(f);

	}

	faces.erase(f->getID());
	//delete f; //TODO -- fix this (needs a virtual destructor)


}



// =======================================================================
// the load function parses very simple .obj files
// the basic format has been extended to allow the specification 
// of crease weights on the edges.
// =======================================================================

#define MAX_CHAR_PER_LINE 200

void Mesh::Load(const std::string &input_file) {

	std::ifstream istr(input_file.c_str());
	if (!istr) {
		std::cout << "ERROR! CANNOT OPEN: " << input_file << std::endl;
		return;
	}

	char line[MAX_CHAR_PER_LINE];
	std::string token, token2;
	float x,y,z;
	int a,b,c,d;
	int index = 0;
	int vert_count = 0;
	int vert_index = 1;

	// read in each line of the file
	while (istr.getline(line,MAX_CHAR_PER_LINE)) { 
		// put the line into a stringstream for parsing
		std::stringstream ss;
		ss << line;

		// check for blank line
		token = "";	 
		ss >> token;
		if (token == "") continue;

		if (token == std::string("usemtl") ||
			token == std::string("g")) {
			vert_index = 1; 
			index++;
		} 
		else if (token == std::string("v")) {
			vert_count++;
			ss >> x >> y >> z;
			addVertex(glm::vec3(x,y,z));
		} 
		else if (token == std::string("f")) {
			a = b = c = d =-1;
			ss >> a >> b >> c;
			int a_ = a-vert_index;
			int b_ = b-vert_index;
			int c_ = c-vert_index;
			assert (a_ >= 0 && a_ < numVertices());
			assert (b_ >= 0 && b_ < numVertices());
			assert (c_ >= 0 && c_ < numVertices());
//			std::cout << "I found a face =]" << std::endl;
			//if it's a quad
			if (ss >> d) {
//				std::cout << "    It's a quad face" <<std::endl;
				int d_ = d-vert_index;
				assert (d_ >= 0 && d_ < numVertices());
				addFace(getVertex(a_),getVertex(b_),getVertex(c_),getVertex(d_));
			}
			//if it's a triangle
			else{
//				std::cout << "    It's a triangle face" << std::endl;
				addFace(getVertex(a_),getVertex(b_),getVertex(c_));
			} 
			//if it has more than 4 sides (turn the rest into a triangle fan)
			b = d;
			while(ss >> c){
				int a_ = a-vert_index;
				int b_ = b-vert_index;
				int c_ = c-vert_index;
				assert (a_ >= 0 && a_ < numVertices());
				assert (b_ >= 0 && b_ < numVertices());
				assert (c_ >= 0 && c_ < numVertices());
				addFace(getVertex(a_),getVertex(b_),getVertex(c_));
				b = c;
			}
		} 
		else if (token == std::string("e")) {
			//ignore edges
		} 
		else if (token == std::string("vt")) {
		} 
		else if (token == std::string("vn")) {
		} 
		else if (token[0] == '#') {
		} 
		else {
			printf ("LINE: '%s'",line);
		}
	}

	std::cout << "Loaded " << numFaces() << " faces." << std::endl;

	assert (numFaces() > 0);
	num_mini_triangles = 0;
	//std::cout << "Mesh quality is " << calculateMeshQuality() <<std::endl;
}


// =======================================================================
// DRAWING
// =======================================================================

glm::vec3 ComputeNormal(const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3) {
	glm::vec3 v12 = p2;
	v12 -= p1;
	glm::vec3 v23 = p3;
	v23 -= p2;
	glm::vec3 normal = glm::cross(v12,v23);
	normal = glm::normalize(normal);
	return normal;
}


void Mesh::initializeVBOs() {
	HandleGLError("enter initialize VBOs");

	// create a pointer for the vertex & index VBOs
	glGenVertexArrays(1, &mesh_VAO);
	glBindVertexArray(mesh_VAO);
	glGenBuffers(1, &mesh_tri_verts_VBO);
	glGenBuffers(1, &mesh_tri_indices_VBO);
	// and the data to pass to the shaders
	GLCanvas::MatrixID = glGetUniformLocation(GLCanvas::programID, "MVP");
	GLCanvas::LightID = glGetUniformLocation(GLCanvas::programID, "LightPosition_worldspace");
	GLCanvas::ViewMatrixID = glGetUniformLocation(GLCanvas::programID, "V");
	GLCanvas::ModelMatrixID = glGetUniformLocation(GLCanvas::programID, "M");

	GLCanvas::wireframeID = glGetUniformLocation(GLCanvas::programID, "wireframe");

	// call this the first time...
	setupVBOs();
	HandleGLError("leaving initializeVBOs");
}


void Mesh::TriVBOHelper( std::vector<glm::vec3> &indexed_verts,
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
												 const glm::vec3 &center_color) {

	// To create a wireframe rendering...
	// Each mesh triangle is actually rendered as 3 small triangles
	//					 b
	//					/|\
	//				 / | \
	//				/	|	\
	//			 /	 |	 \	 
	//			/		|		\	
	//		 /		.'.		\	 
	//		/	.'		 '.	\	
	//	 /.'					 '.\ 
	//	a-----------------c
	//
	
	// the center is white, the colors of the two vertices depend on
	// whether the edge is a boundary edge (red) or crease edge (yellow)
	//glm::vec3 center_color(1,1,1);

	// use simple averaging to find centroid & average normal
	glm::vec3 centroid = 1.0f / 3.0f * (pos_a + pos_b + pos_c);
	glm::vec3 normal = normal_a + normal_b + normal_c;
	glm::normalize(normal);

	int i = indexed_verts.size()/3;

	if (args->wireframe) {
		// WIREFRAME

		// make the 3 small triangles
		indexed_verts.push_back(pos_a);
		indexed_verts.push_back(normal_a);
		indexed_verts.push_back(color_ab);
		indexed_verts.push_back(pos_b);
		indexed_verts.push_back(normal_b);
		indexed_verts.push_back(color_ab);
		indexed_verts.push_back(centroid);
		indexed_verts.push_back(normal);
		indexed_verts.push_back(center_color);
		
		indexed_verts.push_back(pos_b);
		indexed_verts.push_back(normal_b);
		indexed_verts.push_back(color_bc);
		indexed_verts.push_back(pos_c);
		indexed_verts.push_back(normal_c);
		indexed_verts.push_back(color_bc);
		indexed_verts.push_back(centroid);
		indexed_verts.push_back(normal);
		indexed_verts.push_back(center_color);
		
		indexed_verts.push_back(pos_c);
		indexed_verts.push_back(normal_c);
		indexed_verts.push_back(color_ca);
		indexed_verts.push_back(pos_a);
		indexed_verts.push_back(normal_a);
		indexed_verts.push_back(color_ca);
		indexed_verts.push_back(centroid);
		indexed_verts.push_back(normal);
		indexed_verts.push_back(center_color);
		
		// add all of the triangle vertices to the indices list
		for (int j = 0; j < 9; j++) {
			mesh_tri_indices.push_back(i+j);
		}
	} else {
		// NON WIREFRAME
		// Note: gouraud shading with the mini triangles looks bad... :(
		
		// make the 1 triangles
		indexed_verts.push_back(pos_a);
		indexed_verts.push_back(normal_a);
		indexed_verts.push_back(center_color);
		indexed_verts.push_back(pos_b);
		indexed_verts.push_back(normal_b);
		indexed_verts.push_back(center_color);
		indexed_verts.push_back(pos_c);
		indexed_verts.push_back(normal_c);
		indexed_verts.push_back(center_color);
 
		// add all of the triangle vertices to the indices list
		for (int j = 0; j < 3; j++) {
			mesh_tri_indices.push_back(i+j);
		}
	}

}


void Mesh::setupVBOs() {
	HandleGLError("enter setupVBOs");

	std::vector<glm::vec3> indexed_verts;
	std::vector<unsigned int> mesh_tri_indices;
	 
	// write the vertex & triangle data
	for (faceshashtype::iterator iter = faces.begin();
			 iter != faces.end(); iter++) {

		Face* f = iter->second;

		// grab the vertex positions
		glm::vec3 a = (*f)[0]->getPos();
		glm::vec3 b = (*f)[1]->getPos();
		glm::vec3 c = (*f)[2]->getPos();
		glm::vec3 normal = ComputeNormal(a,b,c);

		if(f->is_triangle_face()){
			glm::vec3 edgecolor_ab = black;
			glm::vec3 edgecolor_bc = black;
			glm::vec3 edgecolor_ca = black;

			TriVBOHelper(indexed_verts,mesh_tri_indices,
								 a,b,c,
								 normal,normal,normal,
								 edgecolor_ab,edgecolor_bc,edgecolor_ca,
								 f->getColor());
		}
		else if(f->is_quad_face()){
			glm::vec3 d = (*f)[3]->getPos();

			//first triangle: abc
			glm::vec3 edgecolor_ab = black;
			glm::vec3 edgecolor_bc = black;
			glm::vec3 edgecolor_ca = f->getColor();;

			TriVBOHelper(indexed_verts,mesh_tri_indices,
								 a,b,c,
								 normal,normal,normal,
								 edgecolor_ab,edgecolor_bc,edgecolor_ca,
								 f->getColor());

			//second triangle
			glm::vec3 edgecolor_da = black;
			glm::vec3 edgecolor_cd = black;

			TriVBOHelper(indexed_verts,mesh_tri_indices,
								 a,c,d,
								 normal,normal,normal,
								 edgecolor_ca,edgecolor_cd,edgecolor_da,
								 f->getColor());



		}
		else{
			assert(0); //not quad or triangle?
		}
		
	}
				
	// the vertex data
	glBindBuffer(GL_ARRAY_BUFFER, mesh_tri_verts_VBO);
	glBufferData(GL_ARRAY_BUFFER, indexed_verts.size() * sizeof(glm::vec3), &indexed_verts[0], GL_STATIC_DRAW);
	// the index data (refers to vertex data)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_tri_indices_VBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_tri_indices.size() * sizeof(unsigned int), &mesh_tri_indices[0] , GL_STATIC_DRAW);

	num_mini_triangles = mesh_tri_indices.size();

	HandleGLError("leaving setupVBOs");
}


void Mesh::drawVBOs(const glm::mat4 &ProjectionMatrix,const glm::mat4 &ViewMatrix,const glm::mat4 &ModelMatrix) {
	HandleGLError("enter drawVBOs");

	// prepare data to send to the shaders
	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

	glm::vec3 lightPos = glm::vec3(4,4,4);
	glUniform3f(GLCanvas::LightID, lightPos.x, lightPos.y, lightPos.z);
	glUniformMatrix4fv(GLCanvas::MatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(GLCanvas::ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(GLCanvas::ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
	glUniform1i(GLCanvas::wireframeID, args->wireframe);

	// triangle vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, mesh_tri_verts_VBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,			// attribute
			3,							// size
			GL_FLOAT,					// type
			GL_FALSE,					// normalized?
			3*sizeof(glm::vec3),		// stride
			(void*)0					// array buffer offset
												);
	// triangle vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,			// attribute
			3,							// size
			GL_FLOAT,					// type
			GL_FALSE,					// normalized?
			3*sizeof(glm::vec3),		// stride
			(void*)sizeof(glm::vec3)	// array buffer offset
												);
	// triangle vertex colors
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2,			// attribute
			3,							// size
			GL_FLOAT,					// type
			GL_FALSE,					// normalized?
			3*sizeof(glm::vec3),		// stride
			(void*)(sizeof(glm::vec3)*2)// array buffer offset
												);
	// triangle indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_tri_indices_VBO);
	glDrawElements(GL_TRIANGLES,		// mode
			num_mini_triangles*3,		// count
			GL_UNSIGNED_INT,			// type
			(void*)0					// element array buffer offset
												);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	// =================================
	// draw the different types of edges
	//	if (args->wireframe) {

	HandleGLError("leaving drawVBOs");
}

double Mesh::calculateMeshQuality(){
	double quality = 0;

	//avoid divide by zero error for an empty mesh
	if(faces.size() == 0){
		return quality;
	}
	//sum up all of the individual faces quality
	for(faceshashtype::iterator f = faces.begin(); f != faces.end(); f++){
		Face* current_face = f->second;
		//this function setst the color, so it doesn't have to be done here
		quality += current_face->quality(true);

	}
	//get the average quality
	quality /= faces.size();
	std::cout << "Mesh quality is " << quality << std::endl;

	return quality;
}

void Mesh::showFaceTypes(){
	for(faceshashtype::iterator f = faces.begin(); f != faces.end(); f++){
		Face* current_face = f->second;
		if(current_face->is_triangle_face()){
			current_face->setColor(glm::vec3(1,0,1)); //all triangles purple
		}
		else{ //if is quad
			current_face->setColor(glm::vec3(0,1,1)); //all quads teal
		}
	}
}

void Mesh::refine_mesh_delaunay(){
	int num_swaps = 0;
	std::cout << "Delaunay Mesh Refinement!" << std::endl;
	bool still_improving = true;
	
	while(still_improving){
//		std::cout << "   Delaunay iteration" << std::endl;
		still_improving = false;
		for(faceshashtype::iterator f = faces.begin(); f != faces.end(); f++){
//			std::cout << "    next face" << std::endl;
			Face* current_face = f->second;
			//delaunay is for triangles
			if(current_face->is_triangle_face()){
				std::vector<Face*> adjacent_faces = current_face->get_adjacent_faces();

				//see if we should delaunize with each face.  Break if we did (because this face changed)
				for(unsigned int a = 0; a < adjacent_faces.size(); a++){
//					std::cout << "      this is an adjacent face" <<std::endl;
					if(adjacent_faces[a]->is_triangle_face()){ //mush both be triangles
						bool swapped_diagonal = delaunay(current_face, adjacent_faces[a]);
						if(swapped_diagonal){
							still_improving = true;
							num_swaps++;
							break;
						}
					}

				}
			}

			if(still_improving) break;
		}
	}

	std::cout << "There were " << num_swaps << " swaps made in this mesh refinement!!" << std::endl;
}

bool Mesh::delaunay(Face* face1, Face* face2){
	assert(face1->is_triangle_face() && face2->is_triangle_face());
//	std::cout << "        face are " << face1->getID() << " and " << face2->getID() << std::endl;
//	std::cout << "        vertices are: face1: (" << (*face1)[0]->getIndex() << " " << (*face1)[1]->getIndex() << " " << (*face1)[2]->getIndex() << ") face2: ("
//							 << (*face2)[0]->getIndex() << " " << (*face2)[1]->getIndex() << " " << (*face2)[2]->getIndex() << " )" << std::endl; 

	glm::vec3 face1_normal = ComputeNormal((*face1)[0]->getPos(), (*face1)[1]->getPos(), (*face1)[2]->getPos());
	glm::vec3 face2_normal = ComputeNormal((*face2)[0]->getPos(), (*face2)[1]->getPos(), (*face2)[2]->getPos());

//	std::cout << "        normals are face1: (" << face1_normal.x << " " << face1_normal.y << " " << face1_normal.x << ") face2: ("
//							 << face2_normal.x << " " << face2_normal.y << " " << face2_normal.z << " )" << std::endl; 



	double original_angle = acos( glm::dot(face1_normal, face2_normal) );

//	std::cout << "        angle between these two faces is " << original_angle << std::endl;
	//triangles are are at too great of an angle to combine
	if(original_angle > ANGLE_THRESHOLD) return false;

	//find the vertices that the faces have in common (and not in common)
	std::vector<bool> vert_in_face2;
	std::vector<Vertex*> vertices_in_common;
	Vertex* just_in_face1;
	Vertex* just_in_face2;

	//get verts in common and the one just in face 1
	for(int v = 0; v < 3; v++){
		if(face2->has_vertex( (*face1)[v] ) ){
			vertices_in_common.push_back((*face1)[v]);
			vert_in_face2.push_back(true);
		}
		else{
			just_in_face1 = ( (*face1)[v] );
			vert_in_face2.push_back(false);
		}
	}
	assert(vertices_in_common.size() == 2);

	//get the vert that is just in face 2
	for(int v = 0; v < 3; v++){
		if(! (face1->has_vertex( (*face2)[v] )) ){
			just_in_face2 = (*face2)[v];
		}
	}
	
	//if the vertices are the first and the third, they were put into the vector in the wrong order
	if(vert_in_face2[1] == false){
		Vertex* tmp = vertices[0];
		vertices[0] = vertices[1];
		vertices[1] = tmp;
	}
	
	//test to see if we should switch
	Face* new_face_a = new TriangleFace();
	Vertex** a_verts = new Vertex* [3];
	a_verts[0] = just_in_face1;
	a_verts[1] = just_in_face2;
	a_verts[2] = vertices_in_common[1];
	new_face_a->setVertices(3, a_verts);

	Face* new_face_b = new TriangleFace();
	Vertex** b_verts = new Vertex* [3];
	b_verts[0] = just_in_face1;
	b_verts[1] = vertices_in_common[0];
	b_verts[2] = just_in_face2;
	new_face_b->setVertices(3, b_verts);


	glm::vec3 facea_normal = ComputeNormal(a_verts[0]->getPos(), a_verts[1]->getPos(), a_verts[2]->getPos());
	glm::vec3 faceb_normal = ComputeNormal(b_verts[0]->getPos(), b_verts[1]->getPos(), b_verts[2]->getPos());
	double new_angle = acos(glm::dot(facea_normal, faceb_normal));

	double angle_between_b_1 = acos(glm::dot(facea_normal, face1_normal));
	double angle_between_a_1 = acos(glm::dot(faceb_normal, face1_normal));

	//make sure the new triangles' normals are not too different from each other
	if(new_angle > ANGLE_THRESHOLD) return false;


	
	//this is a hack to make sure triangles are facing the right way (I'm sorry)
	if(angle_between_b_1 > ANGLE_THRESHOLD || angle_between_a_1 > ANGLE_THRESHOLD){
		Vertex* tmp = a_verts[0];
		a_verts[0] = a_verts[2];
		a_verts[2] = tmp;

		tmp = b_verts[0];
		b_verts[0] = b_verts[2];
		b_verts[2] = tmp;

		new_face_a->setVertices(3, a_verts);
		new_face_b->setVertices(3, b_verts);

		facea_normal = ComputeNormal(a_verts[0]->getPos(), a_verts[1]->getPos(), a_verts[2]->getPos());
		faceb_normal = ComputeNormal(b_verts[0]->getPos(), b_verts[1]->getPos(), b_verts[2]->getPos());
		new_angle = acos(glm::dot(facea_normal, faceb_normal));

		angle_between_b_1 = acos(glm::dot(facea_normal, face1_normal));
		angle_between_a_1 = acos(glm::dot(faceb_normal, face1_normal));



		//I don't think this should ever happen because it didn't facing the other direction (outside of the if statement)
		if(new_angle > ANGLE_THRESHOLD) return false;

		//not sure if this should happen
		if(angle_between_b_1 > ANGLE_THRESHOLD || angle_between_a_1 > ANGLE_THRESHOLD){
			return false;
		}


	}

//	std::cout << "        IF WE IMPROVE, WE'RE SWAPPING!!!" << std::endl;


	//both angles are close enough, so if the diagonal swap improves quality, we want to do it
	double old_quality = face1->quality(false) + face2->quality(false);
	double new_quality = new_face_a->quality(false) + new_face_b->quality(false);

//	std::cout << "        Old quality is " << old_quality << " and new quality is " << new_quality << std::endl;

	//should we swap
	if(new_quality > old_quality){
		removeFace(face1);
		removeFace(face2);
		addFace(3, a_verts);
		addFace(3, b_verts);







		return true;
	}


	
	return false;

}

void Mesh::cleanupVBOs() {
	glDeleteBuffers(1, &mesh_VAO);
	glDeleteBuffers(1, &mesh_tri_verts_VBO);
	glDeleteBuffers(1, &mesh_tri_indices_VBO);
}

