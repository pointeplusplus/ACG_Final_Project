#ifndef _FACE_H
#define _FACE_H



#define QUAD_TO_CIRCLE_RATIO 0.636619772
#define TRIANGLE_TO_CIRCLE_RATIO 0.41349667

#include <cmath>
#include <vector>
// ===========================================================
// Stores the indices to the 3 vertices of the triangles, 
// used by the mesh class

class Face {

public:

	// ========================
	// CONSTRUCTOR & DESTRUCTOR
	Face() {
		id = next_face_id;
		next_face_id++;
		color = glm::vec3(0.7,0.7,0.7);
	}

	// =========
	// ACCESSORS
	Vertex* operator[](int i) const { 
		assert (i >= 0 && i < num_vertices);
		return vertices[i];
	}
/*
	glm::vec3 get_mormal(){

		glm::vec3 normal;
		//two vectors in the plane
		glm::vec3 vec_1 = vertices[0]->getPos() - vertices[1]->getPos();
		glm::vec3 vec_2 = 
	}
*/
	int get_vertex_index(int i){
		assert (i >= 0 && i < num_vertices);
		return vertices[i]->getIndex();
	}

	int getID() { return id; }

	glm::vec3 getColor(){ return color;}

	int get_num_vertices() { return num_vertices; }
	virtual bool is_triangle_face() =0;
	virtual bool is_quad_face() =0;
	virtual double area() =0;
	bool has_vertex(Vertex* vert){
		for(int v = 0; v < num_vertices; v++){
			if(vert == vertices[v]){
				return true;
			}
		}
		return false;
	}
	double area_of_circle(Vertex* vert0, Vertex* vert1, Vertex* vert2){
		//TODO: this will cause a divide by zero if they are colinear
		double radius_of_circle = 0;

		//lengths of the 3 sides of the triangle
		double a = glm::length(vert0->getPos()-vert1->getPos());
		double b = glm::length(vert0->getPos()-vert2->getPos());
		double c = glm::length(vert1->getPos()-vert2->getPos());


		radius_of_circle = (a* b * c) / pow( ((2.0*a*a*b*b) + (2.0*b*b*c*c) + (2.0*c*c*a*a) - (a*a*a*a) - (b*b*b*b) - (c*c*c*c)), 0.5);

		return M_PI * pow(radius_of_circle, 2.0);
	}
	virtual double area_of_circumscribed_circle() =0;
	virtual double quality(bool change_color) =0;

	glm::vec3 quality_color(double ratio){
//		std::cout << "quality color ratio is " << ratio << std::endl;
		return glm::vec3(1-ratio, ratio, 0);
	}

	std::vector<Face*> get_adjacent_faces(){

		std::vector<Face*> f;

		for(int a = 0; a < num_vertices; a++){
			if(adjacent_faces[a] != NULL){
				f.push_back(adjacent_faces[a]);
			}
		}

		return f;
	}


	//MODIFIERS
	virtual void setVertices(int num_verts, Vertex** verts) = 0;

	void setColor(glm::vec3 color_val){
		color = color_val;
	}

	void add_adjacency(Face* face_to_add){

//		std::cout << "adding adjacentcy from face " << getID() << " to face " << face_to_add->getID() << std::endl;
		for(int a = 0; a < num_vertices; a++){
			//put new adjacent face in the first open spot we see
			if(adjacent_faces[a] == NULL){
				adjacent_faces[a] = face_to_add;
				return;
			}
		}
		//We should not already have as many adjacent faces as there are vertices
		//BREAKS WITH A BAD MESH
		//assert(0);
	}

	void remove_adjacency(Face* face_to_remove){
		
		//if we find it in the middle, we leave a hole in the array
		for(int a = 0; a < num_vertices; a++){
			if(adjacent_faces[a] == face_to_remove){
				adjacent_faces[a] = NULL;
			}
		}
	}


protected:

	// don't use these constructors
	Face(const Face &/*t*/) { assert(0); exit(0); }
	Face& operator= (const Face &/*t*/) { assert(0); exit(0); }
	
	// ==============
	// REPRESENTATION
	int id;
	int num_vertices;
	Vertex** vertices;
	Face** adjacent_faces;
	glm::vec3 color;	


	// triangles are indexed starting at 0
	static int next_face_id;
};

#endif

#ifndef _TRIANGLEFACE_H
#define _TRIANGLEFACE_H
class TriangleFace : public Face{

public:
	TriangleFace(){
		vertices = new Vertex*[3];
		num_vertices = 3;
		adjacent_faces = new Face*[3];
		for(int a = 0; a < 3; a++){
			adjacent_faces[a] = NULL;
		}
//		std::cout << "number of vertices is " << num_vertices << std::endl;
	} 

	//Accessors
	bool is_quad_face() {return false;}
	bool is_triangle_face() {return true;}
	double area(){ //TODO
		glm::vec3 side1 = vertices[0]->getPos() - vertices[1]->getPos();
		glm::vec3 side2 = vertices[0]->getPos() - vertices[2]->getPos();

		double area = glm::length( glm::cross(side1, side2) )/2.0;

		return area;
	}
	double area_of_circumscribed_circle(){
		/*added to the area of circle function
		//TODO: this will cause a divide by zero if they are colinear
		double radius_of_circle = 0;

		//lengths of the 3 sides of the triangle
		std::cout << "    Vertices are: 1=" << vertices[0]->getIndex() << " 2= " << vertices[1]->getIndex() << " 3= " << vertices[2]->getIndex() << std::endl;
		double a = glm::length(vertices[0]->getPos()-vertices[1]->getPos());
		double b = glm::length(vertices[0]->getPos()-vertices[2]->getPos());
		double c = glm::length(vertices[1]->getPos()-vertices[2]->getPos());

		std::cout << "    Triangle edges have lengths: " << a << " " << b << " " << c << std::endl;

		radius_of_circle = (a* b * c) / pow( ((2.0*a*a*b*b) + (2.0*b*b*c*c) + (2.0*c*c*a*a) - (a*a*a*a) - (b*b*b*b) - (c*c*c*c)), 0.5);
		std::cout << "    face index is: " << getID() << std::endl;
		std::cout << "    radius of the circle is " << radius_of_circle << std::endl;
		std::cout << "    area of circle is " << M_PI * pow(radius_of_circle, 2.0) << std::endl;
		return M_PI * pow(radius_of_circle, 2.0);
		*/

		return area_of_circle(vertices[0], vertices[1], vertices[2]);
	}

	double quality(bool change_color){

		//ratio of area to area of circomscribed circle
		double ratio = area() / area_of_circumscribed_circle();

		//compare to ideal to make return ratio 0 - 1
		ratio /= TRIANGLE_TO_CIRCLE_RATIO;
//		std::cout << "  quality of triangle face is " << ratio <<std::endl;

		//TODO, move color change
		if(change_color){
			color = quality_color(ratio);
		}
		return ratio;
	}

	//Modifiers
	void setVertices(int num_verts, Vertex** verts){
		assert(num_verts == 3);
		for(int v = 0; v < num_verts; v++){
			vertices[v] = verts[v];
		}
//		std::cout << "in the setVertices function, the number of vertices is " << num_vertices << std::endl;
	}

};
#endif
// ===========================================================

#ifndef _QUADFACE_H
#define _QUADFACE_H
class QuadFace : public Face{

public:
	QuadFace(){
		vertices = new Vertex*[4];
		num_vertices = 4;
		adjacent_faces = new Face*[4];
		for(int a = 0; a < 4; a++){
			adjacent_faces[a] = NULL;
		}
	} 


	//Accessors
	bool is_quad_face() {return true;}
	bool is_triangle_face() {return false;}
	double area(){ 
		//TODO: this function assumes quads are convex (so does printing)
		double area = 0.0;

		//half of the quad
		TriangleFace half_face;
		Vertex* vert_array[3] = {vertices[0],vertices[1], vertices[2]};
		half_face.setVertices(3, vert_array);
		area += half_face.area();
		//other half of the quad
		vert_array[1] = vertices[2];
		vert_array[2] = vertices[3];
		half_face.setVertices(3, vert_array);
		area += half_face.area();
		return area;

	}
	double area_of_circumscribed_circle(){
		double temp_area = 0.0;

		//check to see if each triangle area is the largest
		double area = area_of_circle(vertices[0], vertices[1], vertices[2]);
		temp_area = area_of_circle(vertices[1], vertices[2], vertices[3]);
		if(temp_area > area) area = temp_area;
		temp_area = area_of_circle(vertices[2], vertices[3], vertices[0]);
		if(temp_area > area) area = temp_area;
		temp_area = area_of_circle(vertices[3], vertices[0], vertices[1]);
		if(temp_area > area) area = temp_area;
		return area;
	}
	double quality(bool change_color){
		//ratio of area to area of circomscribed circle
		double ratio = area() / area_of_circumscribed_circle();
//		std::cout << "perfect ratio is " << ratio << std::endl;

		//compare to ideal to make return ratio 0 - 1
		ratio /= QUAD_TO_CIRCLE_RATIO;
//		std::cout << "    quality of quad face is " << ratio << std::endl;

		//TODO, move color change
		if(change_color){
			color = quality_color(ratio);
		}
		return ratio;
	}
	
	//Modifiers
	void setVertices(int num_verts, Vertex** verts){
		assert(num_verts == 4);
		for(int v = 0; v < num_verts; v++){
			vertices[v] = verts[v];
		}
	}

};
#endif
// ===========================================================

