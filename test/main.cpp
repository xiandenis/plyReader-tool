#include "PlyReader.h"
#include <memory>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace SOLUTION;

void plyreader (std::string const& path ,std::string const& path2) 
{
	
	std::unique_ptr<PlyReader> reader (new PlyReader (path) );
	{
		
		const auto r2x = reader->find_element("vertex") ;
		assert(r2x != -1) ;
		const auto r02x = reader->element_size(r2x) ;
		std::cout << "r02x : "  << r02x  << std::endl ;
		const auto r3x = reader->find_property (r2x ,"x") ;
		assert(r3x != -1) ;
		const auto r4x = reader->find_property (r2x , "y") ;
		assert(r4x != -1) ;
		const auto r5x = reader->find_property (r2x , "z") ;
		assert(r5x != -1) ;
		// const auto 
		const auto r6x = reader->find_property (r2x ,"nx") ;
		assert(r6x != -1) ;
		const auto r7x = reader->find_property (r2x , "ny") ;
		assert(r7x != -1) ;
		const auto r8x = reader->find_property (r2x , "nz") ;
		assert(r8x != -1) ;

		const auto r9x = reader->find_property (r2x ,"red") ;
		assert(r9x != -1) ;
		const auto r10x = reader->find_property (r2x , "green") ;
		assert(r10x != -1) ;
		const auto r11x = reader->find_property (r2x , "blue") ;
		assert(r11x != -1) ;

		std::ofstream out(path2.c_str(), std::ios::binary);
		out << "ply" << "\n";
		out << "format binary_little_endian 1.0" << "\n";
		out << "comment Export generated by 4dkk" << "\n";
		out << "element vertex " << r02x << "\n";
		out << "property float x" << "\n";
		out << "property float y" << "\n";
		out << "property float z" << "\n";
		out << "property float nx" << "\n";
		out << "property float ny" << "\n";
		out << "property float nz" << "\n";
		out << "property uchar red" << "\n";
		out << "property uchar green" << "\n";
		out << "property uchar blue" << "\n";
		out << "end_header" << "\n";

		for (int i = 0 ; i < r02x ; ++i)
		{

			float v[3];
			float n[3];
			unsigned char color[3];
			v[0] = static_cast<float> (reader->get_value (r2x , i ,r3x)) ;
			v[1] = static_cast<float> (reader->get_value (r2x , i ,r4x)) ;
			v[2] = static_cast<float> (reader->get_value (r2x , i ,r5x)) ;
			n[0] = static_cast<float> (reader->get_value (r2x , i ,r6x)) ;
			n[1] = static_cast<float> (reader->get_value (r2x , i ,r7x)) ;
			n[2] = static_cast<float> (reader->get_value (r2x , i ,r8x)) ;
			color[0] = static_cast<unsigned char> (reader->get_byte (r2x , i ,r9x)) ;
			color[1] = static_cast<unsigned char> (reader->get_byte (r2x , i ,r10x)) ;
			color[2] = static_cast<unsigned char> (reader->get_byte (r2x , i ,r11x)) ;

			out.write((char const*)v, 3 * sizeof(float));
			out.write((char const*)n, 3 * sizeof(float));
			out.write((char const*)color, 3);
		}

		out.close ();
	}

}

int main () 
{
	clock_t start = clock ();

	plyreader ("/media/dage/a941eaf0-d161-47b1-8204-fc4d5651946e/date/laser_data/debug101/caches/resampling/Resampling_52.ply" ,
				"/media/dage/a941eaf0-d161-47b1-8204-fc4d5651946e/date/laser_data/debug101/caches/resampling/dengdexian.ply");
	clock_t end = clock ();

	std::cout << double(end - start) / CLOCKS_PER_SEC << "s" << std::endl ;
	return 0 ;
}