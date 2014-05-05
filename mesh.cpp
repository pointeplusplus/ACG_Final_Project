#include "glCanvas.h"
#include <fstream>
#include <sstream>
#include <cmath>

#include "mesh.h"
#include "vertex.h"
#include "face.h"


#define ANGLE_EPSILON 0.1 //From Barb's code
#define ANGLE_THRESHOLD 0.35 //about 20 degrees
//#define ANGLE_THRESHOLD 0.7 //about 40 degrees
//#define ANGLE_THRESHOLD 500 // no threshold

// to give a unique id number to each triangles
int Face::next_face_id = 0;

glm::vec3 black(0.0,0.0,0.0);
glm::vec3 white(1.0,1.0,1.0);

//helper (in face.h)
double angle_between(Vertex* p_left, Vertex* p_middle, Vertex* p_right);

//assumes that both faces are triangles
int verts_in_common(Face* face1, Face* face2){
	int in_common = 0;

	for(int face1_v = 0; face1_v < 3; face1_v++){
		for(int face2_v = 0; face2_v < 3; face2_v++){
			if( (*face1)[face1_v] == (*face2)[face2_v] ){
				in_common++;
			}
		}
	}

	return in_common;
}

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

Face* Mesh::addFace(int num_verts, Vertex** verts) {
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
	return f;
	
}

Face* Mesh::addFace(Vertex* p1, Vertex* p2, Vertex* p3){
	Face* f = new TriangleFace();
	Vertex* verts[3]= {p1, p2, p3};
	f->setVertices(3, verts);
//	std::cout << " made triangle with " << f->get_num_vertices() << " vertices" << std::endl;
	assert (faces.find(f->getID()) == faces.end());
	faces[f->getID()] = f;
	add_adjacency(f);
	return f;
}
Face* Mesh::addFace(Vertex* p1, Vertex* p2, Vertex* p3, Vertex* p4){
	Face* f = new QuadFace();
	Vertex* verts[4]= {p1, p2, p3,p4};
	f->setVertices(4, verts);
	assert (faces.find(f->getID()) == faces.end());
	faces[f->getID()] = f;
	add_adjacency(f);
	return f;
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
//				std::cout << "		It's a quad face" <<std::endl;
				int d_ = d-vert_index;
				assert (d_ >= 0 && d_ < numVertices());
				addFace(getVertex(a_),getVertex(b_),getVertex(c_),getVertex(d_));
			}
			//if it's a triangle
			else{
//				std::cout << "		It's a triangle face" << std::endl;
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
	int num_modifications = 0;
	std::cout << "Mesh Refinement!" << std::endl;
	bool still_improving = true;
	
	while(still_improving){
//		std::cout << "	 Delaunay iteration" << std::endl;
		still_improving = false;
		for(faceshashtype::iterator f = faces.begin(); f != faces.end(); f++){
//			std::cout << "		next face" << std::endl;
			Face* current_face = f->second;
			//delaunay is for triangles
			if(current_face->is_triangle_face()){
				std::vector<Face*> adjacent_faces = current_face->get_adjacent_faces();

				//see if we should delaunize with each face.	Break if we did (because this face changed)
				for(unsigned int a = 0; a < adjacent_faces.size(); a++){
//					std::cout << "			this is an adjacent face" <<std::endl;
					if(adjacent_faces[a]->is_triangle_face()){ //mush both be triangles
						bool swapped_diagonal = delaunay(current_face, adjacent_faces[a]);
						if(swapped_diagonal){
							still_improving = true;
							num_modifications++;
							break;
						}
					}
					else if(adjacent_faces[a]->is_quad_face()){
						bool shapes_changed = triangle_quad_mushing(current_face, adjacent_faces[a]); 
						if(shapes_changed){
							still_improving = true;
							num_modifications++;
							break;
						}
					}

				}
			}

			if(still_improving) break;
		}
	}

	std::cout << "There were " << num_modifications << " modefications made in this mesh refinement!!" << std::endl;
}

bool Mesh::delaunay(Face* face1, Face* face2){

	bool diagonal_swap_allowed = true;
	assert(face1->is_triangle_face() && face2->is_triangle_face());
//	std::cout << "				face are " << face1->getID() << " and " << face2->getID() << std::endl;
//	std::cout << "				vertices are: face1: (" << (*face1)[0]->getIndex() << " " << (*face1)[1]->getIndex() << " " << (*face1)[2]->getIndex() << ") face2: ("
//							 << (*face2)[0]->getIndex() << " " << (*face2)[1]->getIndex() << " " << (*face2)[2]->getIndex() << " )" << std::endl; 



	//This is the part for diagonal swapping
	glm::vec3 face1_normal = ComputeNormal((*face1)[0]->getPos(), (*face1)[1]->getPos(), (*face1)[2]->getPos());
	glm::vec3 face2_normal = ComputeNormal((*face2)[0]->getPos(), (*face2)[1]->getPos(), (*face2)[2]->getPos());

//	std::cout << "				normals are face1: (" << face1_normal.x << " " << face1_normal.y << " " << face1_normal.x << ") face2: ("
//							 << face2_normal.x << " " << face2_normal.y << " " << face2_normal.z << " )" << std::endl; 



	double original_angle = acos( glm::dot(face1_normal, face2_normal) );

//	std::cout << "				angle between these two faces is " << original_angle << std::endl;
	//triangles are are at too great of an angle to combine

	if(original_angle > ANGLE_THRESHOLD){
		diagonal_swap_allowed = false;
	} 

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
	if(new_angle > ANGLE_THRESHOLD){
		diagonal_swap_allowed = false;
	} 

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
		if(new_angle > ANGLE_THRESHOLD){
			diagonal_swap_allowed = false;
		} 

		//not sure if this should happen
		if(angle_between_b_1 > ANGLE_THRESHOLD || angle_between_a_1 > ANGLE_THRESHOLD){
			diagonal_swap_allowed = false;
		}


	}

	//both angles are close enough, so if the diagonal swap improves quality, we want to do it
	double old_quality = face1->quality(false) + face2->quality(false);
	double diagonal_swap_new_quality = new_face_a->quality(false) + new_face_b->quality(false);

//	std::cout << "				Old quality is " << old_quality << " and new quality is " << new_quality << std::endl;

	//SEE IF QUAD SWAPPING IS ALLOWED=========================================================
	bool allowed_to_make_quad = true;

	if(!makes_convex_quad(face1, face2) ){
		allowed_to_make_quad = false;
	}
	Face* new_face_c = new QuadFace();
	Vertex** c_verts = new Vertex*[4];
	c_verts[0] = just_in_face1;
	c_verts[1] = vertices_in_common[0];
	c_verts[2] = just_in_face2;
	c_verts[3] = vertices_in_common[1];
	new_face_c->setVertices(4, c_verts);


	
	//this is actually the normal of one of the 2 triangles if this shape isn't planer
	glm::vec3 facec_normal = ComputeNormal(c_verts[0]->getPos(), c_verts[1]->getPos(), c_verts[2]->getPos());
	double angle_between_c_1 = acos( glm::dot(facec_normal, face1_normal) );
	double angle_between_c_2 = acos( glm::dot(facec_normal, face2_normal) );


	//same hack as the triangles (this only happens if other quad is unacceptable)
	if(angle_between_c_1 > ANGLE_THRESHOLD || angle_between_c_2 > ANGLE_THRESHOLD){


		//starting from the other side
		allowed_to_make_quad = true;
		
		//swap the order of the verts
		Vertex* tmp = c_verts[0];
		c_verts[0] = c_verts[3];
		c_verts[3] = tmp;
		tmp = c_verts[1];
		c_verts[1] = c_verts[2];
		c_verts[2] = tmp;
		new_face_c->setVertices(4, c_verts);

		if(!makes_convex_quad(face1, face2) ){
			allowed_to_make_quad = false;
		}

		facec_normal = ComputeNormal(c_verts[0]->getPos(), c_verts[1]->getPos(), c_verts[2]->getPos());
		angle_between_c_1 = acos( glm::dot(facec_normal, face1_normal) );
		angle_between_c_2 = acos( glm::dot(facec_normal, face2_normal) );

		//I turned it around and it's /still/ not good
		if(angle_between_c_1 > ANGLE_THRESHOLD || angle_between_c_2 > ANGLE_THRESHOLD){
			allowed_to_make_quad = false;
		}

	}
	double create_quad_new_quality = new_face_c->quality(false);

	//==========================================================================================

	assert(old_quality >= 0); //old quality should be non-negative
	
	//if an operation isn't allowed, make the quality generated by that = -1, so we don't have to worry about checking this later
	if(!diagonal_swap_allowed){
		diagonal_swap_new_quality = -1;
	}
	if(!allowed_to_make_quad){
		create_quad_new_quality = -1;
	}
	else{
//		std::cout << std::endl << std::endl << "creating a quad was allowed" <<std::endl << std::endl;
	}
	
	//shoucd we make a quad
	if(create_quad_new_quality > old_quality && create_quad_new_quality > diagonal_swap_new_quality){
//		std::cout << std::endl << std::endl << "making a quad!!" <<std::endl << std::endl;
		removeFace(face1);
		removeFace(face2);
		Face* f1 = addFace(4, c_verts);
		f1->setColor(glm::vec3(0.3, 0, 0.8)); //dark purple

		return true;
	}

	//should we swap diagonals
	if(diagonal_swap_new_quality > old_quality && diagonal_swap_new_quality > create_quad_new_quality){
		removeFace(face1);
		removeFace(face2);
		Face* f1 = addFace(3, a_verts);
		Face* f2 = addFace(3, b_verts);
		f1->setColor(glm::vec3(1,0,1)); //bright purple
		f2->setColor(glm::vec3(1,0,1));

		return true;
	}


	
	return false;

}

//Note: this should only be called with triangle faces
bool Mesh::makes_convex_quad(Face* face1, Face* face2){

	 
	//find the vertices that the faces have in common (and not in common)
	//std::vector<bool> vert_in_face2;
	std::vector<Vertex*> vertices_in_common;
	Vertex* just_in_face1;
	Vertex* just_in_face2;

	//get verts in common and the one just in face 1
	for(int v = 0; v < 3; v++){
		if(face2->has_vertex( (*face1)[v] ) ){
			vertices_in_common.push_back((*face1)[v]);
//			vert_in_face2.push_back(true);
		}
		else{
			just_in_face1 = ( (*face1)[v] );
//			vert_in_face2.push_back(false);
		}
	}
	assert(vertices_in_common.size() == 2);

	//get the vert that is just in face 2
	for(int v = 0; v < 3; v++){
		if(! (face1->has_vertex( (*face2)[v] )) ){
			just_in_face2 = (*face2)[v];
		}
	}

	//check to make sure no angles made by the verts in common are over 90
	//Note: some convex polygons will still fail, but th
	//fist vertex shared
	double angle_sum = angle_between(just_in_face1, vertices_in_common[0], vertices_in_common[1]);
	angle_sum += angle_between(just_in_face2, vertices_in_common[0], vertices_in_common[1]);
	if(angle_sum > M_PI){
		return false;
	}

	angle_sum = angle_between(just_in_face1, vertices_in_common[1], vertices_in_common[0]);
	angle_sum += angle_between(just_in_face2, vertices_in_common[1], vertices_in_common[0]);
	if(angle_sum > M_PI){
		return false;
	}

	//second vertex shared

	return true;
}

bool Mesh::triangle_quad_mushing(Face* face1, Face* face2){
	assert(face1->is_triangle_face() && face2->is_quad_face() );

	glm::vec3 face1_normal = ComputeNormal((*face1)[0]->getPos(), (*face1)[1]->getPos(), (*face1)[2]->getPos());	

	double old_quality = (face1->quality(false) + face2->quality(false))/2.0;

	//check which way to cut the square is best (for all triangle solution)
	//solution  a/b
	Face* face_a = new TriangleFace();
	Vertex** a_verts = new Vertex*[3];
	a_verts[0] = (*face2)[0];
	a_verts[1] = (*face2)[1];
	a_verts[2] = (*face2)[2]; 
	face_a -> setVertices(3, a_verts);
	Face* face_b = new TriangleFace();
	Vertex** b_verts = new Vertex*[3];
	b_verts[0] = (*face2)[0];
	b_verts[1] = (*face2)[2];
	b_verts[2] = (*face2)[3]; 
	face_b -> setVertices(3, b_verts);

	glm::vec3 facea_normal = ComputeNormal(a_verts[0]->getPos(), a_verts[1]->getPos(), a_verts[2]->getPos());
	glm::vec3 faceb_normal = ComputeNormal(b_verts[0]->getPos(), b_verts[1]->getPos(), b_verts[2]->getPos());
	double a_b_quality = (face1->quality(false) + face_a->quality(false) + face_b->quality(false))/3.0; //turn it all to triangles

	//solution  c/d
	Face* face_c = new TriangleFace();
	Vertex** c_verts = new Vertex*[3];
	c_verts[0] = (*face2)[0];
	c_verts[1] = (*face2)[1];
	c_verts[2] = (*face2)[3]; 
	face_c -> setVertices(3, c_verts);
	Face* face_d = new TriangleFace();
	Vertex** d_verts = new Vertex*[3];
	d_verts[0] = (*face2)[1];
	d_verts[1] = (*face2)[2];
	d_verts[2] = (*face2)[3]; 
	face_d -> setVertices(3, d_verts);

	glm::vec3 facec_normal = ComputeNormal(c_verts[0]->getPos(), c_verts[1]->getPos(), c_verts[2]->getPos());
	glm::vec3 faced_normal = ComputeNormal(d_verts[0]->getPos(), d_verts[1]->getPos(), d_verts[2]->getPos());
	double c_d_quality = (face1->quality(false) + face_c->quality(false) + face_d->quality(false))/3.0;

	//try combining the triangle with face a or face b
	double a_b_1_quality = -1;
	Face* quad1;
	bool quad1_valid = false;
	bool used_face_a = false;

	if(verts_in_common(face1, face_a) == 2){ //face a is adjacent to face 1
		used_face_a = true;
		quad1_valid = combine_two_triangles(face1, face_a, quad1);
		if(acos( glm::dot(face1_normal, facea_normal) ) > ANGLE_THRESHOLD){
			quad1_valid = false;
		}
		if(quad1_valid){ //face a was turned into the quad
			a_b_1_quality = (quad1->quality(false) + face_b->quality(false))/2.0;
		}
	}
	else{ //face b is adjacent to face1
		quad1_valid = combine_two_triangles(face1, face_b, quad1);
		if(acos( glm::dot(face1_normal, faceb_normal) ) > ANGLE_THRESHOLD){
			quad1_valid = false;
		}
		if(quad1_valid){ //face b was turned into the quad
			a_b_1_quality = (quad1->quality(false) + face_a->quality(false))/2.0;
		}
	}
	if(quad1_valid == false) a_b_1_quality = -1;

	//try combining the triangle with face c or face d
	double c_d_1_quality = -1;
	Face* quad2;
	bool quad2_valid = false;
	bool used_face_c = false;

	if(verts_in_common(face1, face_c) == 2){ //face d is adjacent to face 1
		used_face_c = true;
		quad2_valid = combine_two_triangles(face1, face_c, quad2);
		if(acos( glm::dot(face1_normal, facec_normal) ) > ANGLE_THRESHOLD){
			quad2_valid = false;
		}
		if(quad2_valid){ //face c was turned into the quad
			c_d_1_quality = (quad2->quality(false) + face_d->quality(false))/2.0;
		}
	}
	else{ //face d is adjacent to face1
		quad2_valid = combine_two_triangles(face1, face_d, quad2);
		if(acos( glm::dot(face1_normal, faced_normal) ) > ANGLE_THRESHOLD){
			quad2_valid = false;
		}
		if(quad2_valid){ //face d was turned into the quad1
			c_d_1_quality = (quad2->quality(false) + face_c->quality(false))/2.0;
		}
	}
	if(quad2_valid == false) c_d_1_quality = -1;


	//now check to see what we should do
	//best thing is the a/b triangle split of the quad
	if(a_b_quality > old_quality && a_b_quality > c_d_quality && a_b_quality> a_b_1_quality && a_b_quality > c_d_1_quality){
		removeFace(face2);
		Face* a_face = addFace(3, a_verts);
		Face* b_face = addFace(3, b_verts);
		a_face->setColor(glm::vec3(1,1,0));
		b_face->setColor(glm::vec3(1,1,0));
		return true;
	}
	//best thing is the c/d triangle split (don't need to test against things that are above)
	else if(c_d_quality > old_quality && c_d_quality > a_b_1_quality && c_d_quality > c_d_1_quality){
		removeFace(face2);
		Face* c_face = addFace(3, c_verts);
		Face* d_face = addFace(3, d_verts);
		c_face->setColor(glm::vec3(1,1,0));
		d_face->setColor(glm::vec3(1,1,0));
		return true;	
	}
	else if(a_b_1_quality > old_quality &&  a_b_1_quality > c_d_1_quality){
		removeFace(face1);
		removeFace(face2);
		Face* q1 = addFace((*quad1)[0],(*quad1)[1],(*quad1)[2],(*quad1)[3]);
		q1->setColor(glm::vec3(1,1,0));
		if(used_face_a){
			Face* b_face = addFace(3, b_verts);
			b_face ->setColor(glm::vec3(1,1,0));
		}
		else{
			Face* a_face = addFace(3, a_verts);
			a_face ->setColor(glm::vec3(1,1,0));
		}
		return true;
	}
	else if(c_d_1_quality > old_quality){
		removeFace(face1);
		removeFace(face2);
		Face* q2 = addFace((*quad2)[0],(*quad2)[1],(*quad2)[2],(*quad2)[3]);
		q2->setColor(glm::vec3(1,1,0));
		if(used_face_c){
			Face* d_face = addFace(3, d_verts);
			d_face ->setColor(glm::vec3(1,1,0));
		}
		else{
			Face* c_face = addFace(3, c_verts);
			c_face ->setColor(glm::vec3(1,1,0));
		}
		return true;
	}

	return false;
}

bool Mesh::combine_two_triangles(Face* face1, Face* face2, Face* &quad){

	glm::vec3 face1_normal = ComputeNormal((*face1)[0]->getPos(), (*face1)[1]->getPos(), (*face1)[2]->getPos());
	glm::vec3 face2_normal = ComputeNormal((*face2)[0]->getPos(), (*face2)[1]->getPos(), (*face2)[2]->getPos());

	std::vector<Vertex*> vertices_in_common;
	Vertex* just_in_face1;
	Vertex* just_in_face2;

	//get verts in common and the one just in face 1
	for(int v = 0; v < 3; v++){
		if(face2->has_vertex( (*face1)[v] ) ){
			vertices_in_common.push_back((*face1)[v]);

		}
		else{
			just_in_face1 = ( (*face1)[v] );
		}
	}
	assert(vertices_in_common.size() == 2);

	//get the vert that is just in face 2
	for(int v = 0; v < 3; v++){
		if(! (face1->has_vertex( (*face2)[v] )) ){
			just_in_face2 = (*face2)[v];
		}
	}

	//SEE IF QUAD SWAPPING IS ALLOWED=========================================================
	bool allowed_to_make_quad = true;

	if(!makes_convex_quad(face1, face2) ){
		allowed_to_make_quad = false;
	}
	Face* new_face_c = new QuadFace();
	Vertex** c_verts = new Vertex*[4];
	c_verts[0] = just_in_face1;
	c_verts[1] = vertices_in_common[0];
	c_verts[2] = just_in_face2;
	c_verts[3] = vertices_in_common[1];
	new_face_c->setVertices(4, c_verts);


	
	//this is actually the normal of one of the 2 triangles if this shape isn't planer
	glm::vec3 facec_normal = ComputeNormal(c_verts[0]->getPos(), c_verts[1]->getPos(), c_verts[2]->getPos());
	double angle_between_c_1 = acos( glm::dot(facec_normal, face1_normal) );
	double angle_between_c_2 = acos( glm::dot(facec_normal, face2_normal) );


	//same hack as the triangles (this only happens if other quad is unacceptable)
	if(angle_between_c_1 > ANGLE_THRESHOLD || angle_between_c_2 > ANGLE_THRESHOLD){


		//starting from the other side
		allowed_to_make_quad = true;
		
		//swap the order of the verts
		Vertex* tmp = c_verts[0];
		c_verts[0] = c_verts[3];
		c_verts[3] = tmp;
		tmp = c_verts[1];
		c_verts[1] = c_verts[2];
		c_verts[2] = tmp;
		new_face_c->setVertices(4, c_verts);

		if(!makes_convex_quad(face1, face2) ){
			allowed_to_make_quad = false;
		}

		facec_normal = ComputeNormal(c_verts[0]->getPos(), c_verts[1]->getPos(), c_verts[2]->getPos());
		angle_between_c_1 = acos( glm::dot(facec_normal, face1_normal) );
		angle_between_c_2 = acos( glm::dot(facec_normal, face2_normal) );

		//I turned it around and it's /still/ not good
		if(angle_between_c_1 > ANGLE_THRESHOLD || angle_between_c_2 > ANGLE_THRESHOLD){
			allowed_to_make_quad = false;
		}

	}
	quad = new_face_c;
	return allowed_to_make_quad;
}


void Mesh::cleanupVBOs() {
	glDeleteBuffers(1, &mesh_VAO);
	glDeleteBuffers(1, &mesh_tri_verts_VBO);
	glDeleteBuffers(1, &mesh_tri_indices_VBO);
}


