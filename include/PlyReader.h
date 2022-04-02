#pragma once
#include <stdint.h> 
#include <string>
#include <vector>
#include <memory>


using namespace std;

using VAL32 = float;
using VAL64 = double;
using VALXA = double ;

using FLAG = int64_t;
using INDEX = int64_t;
using LENGTH = int64_t;

using BOOL = bool;

using BYTE = uint8_t ;
using WORD = uint16_t ;
using CHAR = uint32_t ;
using DATA = uint64_t;


using VAR32 = int32_t;
using VAR64 = int64_t;

namespace SOLUTION {
class PlyReader {
private:
	using my_value_t = VALXA ;
	using my_index_t = INDEX ;
	using my_byte_t = DATA ;
	using my_string_t = std::string ;


	class Abstract {
	public:
		inline Abstract () = default ;
		inline virtual ~Abstract () = default ;
		inline Abstract (const Abstract &) = delete ;
		inline Abstract &operator= (const Abstract &) = delete ;
		inline Abstract (Abstract &&) = delete ;
		inline Abstract &operator= (Abstract &&) = delete ;
		virtual my_index_t find_element (const my_string_t &name) const = 0 ;
		virtual my_index_t element_size (const my_index_t &element_index) const = 0 ;
		virtual my_index_t find_property (const my_index_t &element_index ,const my_string_t &name) const = 0 ;
		virtual const my_value_t &get_value (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const = 0 ;
		virtual const vector<my_value_t> &get_value_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const = 0 ;
		virtual const my_index_t &get_index (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const = 0 ;
		virtual const vector<my_index_t> &get_index_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const = 0 ;
		virtual const my_byte_t &get_byte (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const = 0 ;
		virtual const vector<my_byte_t> &get_byte_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const = 0 ;
	} ;

	using my_holder_t = std::shared_ptr<Abstract> ;


	class Implement ;

private:
	my_holder_t mPointer ;
	
public:
	PlyReader () = default ;

	explicit PlyReader (const my_string_t &file) {
		mPointer = create (file) ;
	}

	my_index_t find_element (const my_string_t &name) const {
		check_avaliable (mPointer) ;
		return mPointer->find_element (name) ;
	}

	my_index_t element_size (const my_index_t &element_index) const {
		check_avaliable (mPointer) ;
		return mPointer->element_size (element_index) ;
	}

	my_index_t find_property (const my_index_t &element_index ,const my_string_t &name) const {
		check_avaliable (mPointer) ;
		return mPointer->find_property (element_index ,name) ;
	}

	const my_value_t &get_value (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const {
		check_avaliable (mPointer) ;
		return mPointer->get_value (element_index ,line_index ,property_index) ;
	}

	const vector<my_value_t> &get_value_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const {
		check_avaliable (mPointer) ;
		return mPointer->get_value_list (element_index ,line_index ,property_index) ;
	}

	const my_index_t &get_index (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const {
		check_avaliable (mPointer) ;
		return mPointer->get_index (element_index ,line_index ,property_index) ;
	}

	const vector<my_index_t> &get_index_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const {
		check_avaliable (mPointer) ;
		return mPointer->get_index_list (element_index ,line_index ,property_index) ;
	}

	const my_byte_t &get_byte (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const {
		check_avaliable (mPointer) ;
		return mPointer->get_byte (element_index ,line_index ,property_index) ;
	}

	const vector<my_byte_t> &get_byte_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const {
		check_avaliable (mPointer) ;
		return mPointer->get_byte_list (element_index ,line_index ,property_index) ;
	}

private:
	static void check_avaliable (const my_holder_t &pointer) ;

	static my_holder_t create (const my_string_t &file) ;
} ;

} ;
