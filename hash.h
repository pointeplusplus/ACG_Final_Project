#ifndef _HASH_H_
#define _HASH_H_

// to handle different platforms with different variants of a developing standard
// NOTE: You may need to adjust these depending on your installation
#ifdef __APPLE__
// osx mavericks
//#include <unordered_map>
// osx lion
#include <tr1/unordered_map>
// something even older
//#include <ext/hash_map>
#elif defined(_WIN32)
#include <unordered_map>
#elif defined(__linux__)
#include <unordered_map>
#elif defined(__FreeBSD__)
#include <ext/hash_map>
#else
#error "unknown system"
#endif

class Edge;
class Face;
#include "vertex.h"

#define LARGE_PRIME_A 10007
#define LARGE_PRIME_B 11003


// ===================================================================================
// DIRECTED EDGES are stored in a hash table using a simple hash
// function based on the indices of the start and end vertices
// ===================================================================================

inline unsigned int ordered_two_int_hash(unsigned int a, unsigned int b) {
	return LARGE_PRIME_A * a + LARGE_PRIME_B * b;
}

struct orderedvertexpairhash {
	size_t operator()(std::pair<Vertex*,Vertex*> p) const {
		return ordered_two_int_hash(p.first->getIndex(),p.second->getIndex());
	}
};

struct orderedsamevertexpair {
	bool operator()(std::pair<Vertex*,Vertex*> p1, std::pair<Vertex*,Vertex*>p2) const {
		if (p1.first->getIndex() == p2.first->getIndex() && p1.second->getIndex() == p2.second->getIndex())
			return true;
		return false;
	}
};



// ===================================================================================
// PARENT/CHILD VERTEX relationships (for subdivision) are stored in a
// hash table using a simple hash function based on the indices of the
// parent vertices, smaller index first
// ===================================================================================

inline unsigned int unordered_two_int_hash(unsigned int a, unsigned int b) {
	assert (a != b);
	if (b < a) {
		return ordered_two_int_hash(b,a);
	} else {
		assert (a < b);
		return ordered_two_int_hash(a,b);
	}
}

struct unorderedvertexpairhash {
	size_t operator()(std::pair<Vertex*,Vertex*> p) const {
		return unordered_two_int_hash(p.first->getIndex(),p.second->getIndex());
	}
};

struct unorderedsamevertexpair {
	bool operator()(std::pair<Vertex*,Vertex*> p1, std::pair<Vertex*,Vertex*>p2) const {
		if ((p1.first->getIndex() == p2.first->getIndex() && p1.second->getIndex() == p2.second->getIndex()) ||
	(p1.first->getIndex() == p2.second->getIndex() && p1.second->getIndex() == p2.first->getIndex())) return true;
		return false;
	}
};



// ===================================================================================
// TRIANGLES are stored in a hash table using a simple hash function
// based on the id of the triangle (unique ids are assigned when the
// triangle is constructed)
// ===================================================================================

struct idhash {
	size_t operator()(unsigned int id) const {
		return LARGE_PRIME_A * id;
	}
};

struct sameid {
	bool operator()(unsigned int a, unsigned int b) const {
		if (a == b)
			return true;
		return false;
	}
};



// to handle different platforms with different variants of a developing standard
// NOTE: You may need to adjust these depending on your installation
#ifdef __APPLE__
// osx lion
typedef std::tr1::unordered_map<std::pair<Vertex*,Vertex*>,Vertex*,unorderedvertexpairhash,unorderedsamevertexpair> vphashtype;
typedef std::tr1::unordered_map<std::pair<Vertex*,Vertex*>,Edge*,orderedvertexpairhash,orderedsamevertexpair> edgeshashtype;
typedef std::tr1::unordered_map<unsigned int,Face*,idhash,sameid> faceshashtype;

#elif defined(_WIN32)
typedef std::unordered_map<std::pair<Vertex*,Vertex*>,Vertex*,unorderedvertexpairhash,unorderedsamevertexpair> vphashtype;
typedef std::unordered_map<std::pair<Vertex*,Vertex*>,Edge*,orderedvertexpairhash,orderedsamevertexpair> edgeshashtype;
typedef std::unordered_map<unsigned int,Face*,idhash,sameid> faceeshashtype;

#elif defined(__linux__)
typedef std::unordered_map<std::pair<Vertex*,Vertex*>,Vertex*,unorderedvertexpairhash,unorderedsamevertexpair> vphashtype;
typedef std::unordered_map<std::pair<Vertex*,Vertex*>,Edge*,orderedvertexpairhash,orderedsamevertexpair> edgeshashtype;
typedef std::unordered_map<unsigned int,Face*,idhash,sameid> faceshashtype;

#elif defined(__FreeBSD__)
typedef __gnu_cxx::hash_map<std::pair<Vertex*,Vertex*>,Vertex*,unorderedvertexpairhash,unorderedsamevertexpair> vphashtype;
typedef __gnu_cxx::hash_map<std::pair<Vertex*,Vertex*>,Edge*,orderedvertexpairhash,orderedsamevertexpair> edgeshashtype;
typedef __gnu_cxx::hash_map<unsigned int,Face*,idhash,sameid> faceshashtype;

#else
#endif


#endif // _HASH_H_
