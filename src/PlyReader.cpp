
#include "PlyReader.h"
#include <string>
#include <fstream>
#include <deque>
#include <list>
#include <map>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <assert.h>
#include <iostream>
#include "exception.h"
#include "tokenizer.h"
#include "strings.h"
#include <memory>
#include "system.h"

using namespace std;

static constexpr auto PLYREADER_PROPERY_TYPE_NULL = FLAG (0X00) ;
static constexpr auto PLYREADER_PROPERY_TYPE_VAL32 = FLAG (0X10) ;
static constexpr auto PLYREADER_PROPERY_TYPE_VAL64 = FLAG (0X11) ;
static constexpr auto PLYREADER_PROPERY_TYPE_VAR32 = FLAG (0X20) ;
static constexpr auto PLYREADER_PROPERY_TYPE_VAR64 = FLAG (0X21) ;
static constexpr auto PLYREADER_PROPERY_TYPE_BYTE = FLAG (0X30) ;
static constexpr auto PLYREADER_PROPERY_TYPE_WORD = FLAG (0X31) ;
static constexpr auto PLYREADER_PROPERY_TYPE_CHAR = FLAG (0X32) ;
static constexpr auto PLYREADER_PROPERY_TYPE_DATA = FLAG (0X33) ;

static constexpr auto PLYREADER_BODY_TYPE_VALUE = FLAG (0X10) ;
static constexpr auto PLYREADER_BODY_TYPE_INDEX = FLAG (0X11) ;
static constexpr auto PLYREADER_BODY_TYPE_BYTE = FLAG (0X12) ;
static constexpr auto PLYREADER_BODY_TYPE_VALUE_LIST = FLAG (0X0110) ;
static constexpr auto PLYREADER_BODY_TYPE_INDEX_LIST = FLAG (0X0111) ;
static constexpr auto PLYREADER_BODY_TYPE_BYTE_LIST = FLAG (0X0112) ;

enum PLYFormat
{
    PLY_ASCII,
    PLY_BINARY_LE,
    PLY_BINARY_BE,
    PLY_UNKNOWN
};

template <typename T>
T ply_read_value (std::istream& input, PLYFormat format)
{
    T value;
    switch (format)
    {
        case PLY_ASCII:
            input >> value;
            return value;

        case PLY_BINARY_LE:
            input.read(reinterpret_cast<char*>(&value), sizeof(T));
            return util::system::letoh(value);

        case PLY_BINARY_BE:
            input.read(reinterpret_cast<char*>(&value), sizeof(T));
            return util::system::betoh(value);

        default:
            throw std::invalid_argument("Invalid data format");
    }
}


template <>
unsigned char ply_read_value<unsigned char> (std::istream& input, PLYFormat format)
{
    switch (format)
    {
        case PLY_ASCII:
        {
            int temp;
            input >> temp;
            return static_cast<unsigned char>(temp);
        }

        case PLY_BINARY_LE:
        case PLY_BINARY_BE:
        {
            unsigned char value;
            input.read(reinterpret_cast<char*>(&value), 1);
            return value;
        }

        default:
            throw std::invalid_argument("Invalid data format");
    }
}


template <>
char ply_read_value<char> (std::istream& input, PLYFormat format);

template
float ply_read_value<float> (std::istream& input, PLYFormat format);

template
double ply_read_value<double> (std::istream& input, PLYFormat format);


template
unsigned int ply_read_value<unsigned int> (std::istream& input, PLYFormat format);


template
int ply_read_value<int> (std::istream& input, PLYFormat format);


void ply_color_convert (float const* src, unsigned char* dest, int num = 3)
{
    for (int c = 0; c < num; ++c)
    {
        float color = src[c];
        color = color * 255.0f;
        color = std::min(255.0f, std::max(0.0f, color));
        dest[c] = (unsigned char)(color + 0.5f);
    }
}


namespace SOLUTION {
	
class PlyReader::Implement :public Abstract {
private:
	struct PROPERTY {
		std::string mName ;
		FLAG mType ;
		FLAG mListType ;
	} ;

	struct ELEMENT {
		string mName ;
		LENGTH mSize ;
		vector<PROPERTY> mPropertyList ;
		map<string ,INDEX> mPropertyMappingSet ;
	} ;

	struct HEADER {
		string mFormat ;
		vector<ELEMENT> mElementList ;
		map<string ,INDEX> mElementMappingSet ;
		LENGTH mBodyOffset ;
	} ;

private:
	std::ifstream mPlyFile ;
	HEADER mHeader ;
	PLYFormat mBitwiseReverseFlag ;
	vector<vector<vector<INDEX>>> mBody ;
	vector<vector<FLAG>> mBodyType ;
	deque<my_value_t> mPlyValue ;
	deque<vector<my_value_t>> mPlyValueList ;
	deque<my_index_t> mPlyIndex ;
	deque<vector<my_index_t>> mPlyIndexList ;
	deque<my_byte_t> mPlyByte ;
	deque<vector<my_byte_t>> mPlyByteList ;

public:
	Implement () = delete ;

	explicit Implement (const my_string_t &file) {
		if (file.empty())
		{
			throw std::invalid_argument("No filename given");
		}
		mPlyFile.open(file.c_str(), std::ios::binary);
		if (!mPlyFile.good()) 
		{
			throw util::FileException(file, std::strerror(errno));
		}
	
		read_header () ;
		auto fax = true ;
		
		if (fax) 
		{
			if (mHeader.mFormat == "ascii") 
			{
				read_body_text () ;
				fax = false ;
			}
		}

		if (fax) {
			if (mHeader.mFormat == "binary_big_endian") 
			{
				mBitwiseReverseFlag = PLY_BINARY_BE ;
				read_body_binary () ;
				fax = false;
			}	
		}
		
		if (fax) {
			if (mHeader.mFormat == "binary_little_endian")
			{
				mBitwiseReverseFlag = PLY_BINARY_LE ;
				read_body_binary () ;
				fax = false;
			}
		}
		
		if (fax) {
			mPlyFile.close ();
			assert (false) ;
			
		}

		mPlyFile.close ();
	}

	my_index_t find_element (const my_string_t &name) const override {
		const auto r1x = mHeader.mElementMappingSet.find (name) ;
		my_index_t ret = -1 ;
		if (r1x != mHeader.mElementMappingSet.end())
		{
			ret = r1x->second ;
		}
		return ret  ;
	}

	my_index_t element_size (const my_index_t &element_index) const override {
		return  mHeader.mElementList[element_index].mSize ;
	}

	my_index_t find_property (const my_index_t &element_index ,const my_string_t &name) const override {
		my_index_t ret = -1 ;
		const auto r1x = mHeader.mElementList[element_index].mPropertyMappingSet.find (name) ;
		if (r1x != mHeader.mElementList[element_index].mPropertyMappingSet.end ())
		{
			ret = r1x->second ;
		}
		return ret ;
	}

	const my_value_t &get_value (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const override {
		assert (mBodyType[element_index][property_index] == PLYREADER_BODY_TYPE_VALUE) ;
		return mPlyValue[mBody[element_index][line_index][property_index]] ;
	}

	const vector<my_value_t> &get_value_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const override {
		assert (mBodyType[element_index][property_index] == PLYREADER_BODY_TYPE_VALUE_LIST) ;
		return mPlyValueList[mBody[element_index][line_index][property_index]] ;
	}

	const my_index_t &get_index (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const override {
		assert (mBodyType[element_index][property_index] == PLYREADER_BODY_TYPE_INDEX) ;
		return mPlyIndex[mBody[element_index][line_index][property_index]] ;
	}

	const vector<my_index_t> &get_index_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const override {
		assert (mBodyType[element_index][property_index] == PLYREADER_BODY_TYPE_INDEX_LIST) ;
		return mPlyIndexList[mBody[element_index][line_index][property_index]] ;
	}

	const my_byte_t &get_byte (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const override {
		assert (mBodyType[element_index][property_index] == PLYREADER_BODY_TYPE_BYTE) ;
		return mPlyByte[mBody[element_index][line_index][property_index]] ;
	}

	const vector<my_byte_t> &get_byte_list (const my_index_t &element_index ,const my_index_t &line_index ,const my_index_t &property_index) const override {
		assert (mBodyType[element_index][property_index] == PLYREADER_BODY_TYPE_BYTE_LIST) ;
		return mPlyByteList[mBody[element_index][line_index][property_index]] ;
	}

private:
	
	void read_header () {
		
		std::string buffer;
		mPlyFile >> buffer;
		if (buffer != "ply")
		{
			mPlyFile.close () ;
			throw util::Exception("File format not recognized as PLY-model");
		}

		map<string ,int64_t> r1x ;
		{
			r1x.insert (pair < string ,int64_t > ("float" ,PLYREADER_PROPERY_TYPE_VAL32)) ;
			r1x.insert (pair < string ,int64_t > ("double" ,PLYREADER_PROPERY_TYPE_VAL64)) ;
			r1x.insert (pair < string ,int64_t > ("int" ,PLYREADER_PROPERY_TYPE_VAR32)) ;
			r1x.insert (pair < string ,int64_t > ("int64" ,PLYREADER_PROPERY_TYPE_VAR64)) ;
			r1x.insert (pair < string ,int64_t > ("uchar" ,PLYREADER_PROPERY_TYPE_BYTE)) ;
			r1x.insert (pair < string ,int64_t > ("uint16" ,PLYREADER_PROPERY_TYPE_WORD)) ;
			r1x.insert (pair < string ,int64_t > ("uint32" ,PLYREADER_PROPERY_TYPE_CHAR)) ;
			r1x.insert (pair < string ,int64_t > ("uint64" ,PLYREADER_PROPERY_TYPE_DATA)) ;
		}
		
		map<string ,int64_t> r2x ;
		{
			r2x.insert (pair < string ,int64_t > ("int" ,PLYREADER_PROPERY_TYPE_VAR32)) ;
			r2x.insert (pair < string ,int64_t > ("int64" ,PLYREADER_PROPERY_TYPE_VAR64)) ;
			r2x.insert (pair < string ,int64_t > ("uchar" ,PLYREADER_PROPERY_TYPE_BYTE)) ;
			r2x.insert (pair < string ,int64_t > ("uint16" ,PLYREADER_PROPERY_TYPE_WORD)) ;
			r2x.insert (pair < string ,int64_t > ("uint32" ,PLYREADER_PROPERY_TYPE_CHAR)) ;
			r2x.insert (pair < string ,int64_t > ("uint64" ,PLYREADER_PROPERY_TYPE_DATA)) ;
		}

		INDEX ix = -1 ;
		INDEX iy = -1 ;
		while (mPlyFile.good()) 
		{
			std::getline(mPlyFile, buffer);
			util::strings::clip_newlines(&buffer);
			util::strings::clip_whitespaces(&buffer);

			if (buffer == "end_header")
            	break;

			util::Tokenizer header;
			header.split(buffer);

			if (header.empty())
				continue;

			if (header[0] == "format")
			{
				mHeader.mFormat = header[1] ;
			}
			else if (header[0] == "comment") 
			{
				std::cout << "PLY Loader: " << buffer << std::endl;
			}
			else if (header[0] == "element")
			{
				ix = static_cast<INDEX> (mHeader.mElementList.size()) ;
				ELEMENT mTempelement ;
				mTempelement.mName = header[1];
				auto r3x = util::strings::convert<LENGTH>(header[2]);
				mTempelement.mSize = r3x;
				mTempelement.mPropertyList.reserve (r3x);
				// mTempelement.mPropertyMappingSet ;
				mHeader.mElementList.push_back (mTempelement);
			}
			else if (header[0] == "property")
			{
				assert (ix != -1) ;
				if (header[1] == "list") 
				{
					PROPERTY mTempProperty ;
					iy = static_cast<INDEX> (mHeader.mElementList[ix].mPropertyList.size ()) ;
					

					INDEX r4x = -1 ;
					{	
						const auto rd4x = r2x.find (header[2]) ;
						if (rd4x!= r2x.end())
						{
							r4x = rd4x->second ;
						}
					}
					assert (r4x != -1);

					mTempProperty.mType = r4x ;
					// mHeader.mElementList[ix].mPropertyList[iy].mType = r4x ;
					
					INDEX r5x = -1 ;
					{
						const auto rd5x = r1x.find (header[3]) ;
						if (rd5x!= r1x.end())
						{
							r5x = rd5x->second ;
						}

					}
					assert (r5x != -1);

					mTempProperty.mListType = r5x ;
					// mHeader.mElementList[ix].mPropertyList[iy].mListType = r5x ;

					mTempProperty.mName = header[4];

					mHeader.mElementList[ix].mPropertyList.push_back (mTempProperty);
				}
				else 
				{
					PROPERTY mTempProperty ;

					iy = static_cast<INDEX> (mHeader.mElementList[ix].mPropertyList.size ()) ;

					INDEX r6x = -1 ;
					{
						const auto rd6x = r1x.find (header[1]) ;
						if (rd6x!= r1x.end())
						{
							r6x = rd6x->second ;
						}
					}
					assert (r6x != -1);
					
					mTempProperty.mType = r6x ;
					mTempProperty.mListType = PLYREADER_PROPERY_TYPE_NULL ;
					mTempProperty.mName = header[2];

					mHeader.mElementList[ix].mPropertyList.push_back (mTempProperty);
				}
			}
			else 
			{
				assert(false);
			}
		}

		for (auto &&i : mHeader.mElementList) {
			// i.mPropertyMappingSet.(i.mPropertyList.size ()) ;
			for (int j = 0 ; j < (int) i.mPropertyList.size () ; ++j)
			{
				i.mPropertyMappingSet.insert (pair < string ,int64_t > (i.mPropertyList[j].mName ,j)) ;
			}
		}
		
		{
			// mHeader.mElementList.remap () ;
			// mHeader.mElementMappingSet = Set<String<STRU8>> (mHeader.mElementList.length ()) ;
			for (int j = 0 ; j < (int)mHeader.mElementList.size () ; ++j)
			{
				// mHeader.mElementMappingSet.add (mHeader.mElementList[j].mName ,j) ;
				mHeader.mElementMappingSet.insert (pair < string ,int64_t > (mHeader.mElementList[j].mName ,j)) ;
			}
				
		}
	}

	void read_body_text () {

		mBody = vector<vector<vector<INDEX>>> (mHeader.mElementList.size ()) ;
		mBodyType = vector<vector<FLAG>> (mHeader.mElementList.size ()) ;

		for (INDEX i = 0 ; i < (INDEX)mHeader.mElementList.size () ; ++i)
		{
			mBody[i] = vector<vector<INDEX>> (mHeader.mElementList[i].mSize) ;
			mBodyType[i] = vector<FLAG> (mHeader.mElementList[i].mPropertyList.size ()) ;
			for (INDEX k = 0 ; i < (INDEX)mHeader.mElementList[i].mSize ; ++k) 
			{
				mBody[i][k] = vector<INDEX> (mHeader.mElementList[i].mPropertyList.size ()) ;
				for (INDEX j = 0 ; j < (INDEX) mHeader.mElementList[i].mPropertyList.size () ; ++j)
				{
					const auto r3x = mHeader.mElementList[i].mPropertyList[j].mType ;
					const auto r4x = mHeader.mElementList[i].mPropertyList[j].mListType ;
					
					if (true) 
					{
						
						if (r3x == PLYREADER_PROPERY_TYPE_VAL32) 
						{
							INDEX ix = (INDEX) mPlyValue.size () ;
							
							const auto r5x = ply_read_value<float>(mPlyFile, PLY_ASCII);

							mPlyValue.push_back (my_value_t (r5x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_VALUE ;
							mBody[i][k][j] = ix ;
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_VAL64) 
						{
							INDEX ix = (INDEX) mPlyValue.size ();
							const auto r6x = ply_read_value<double>(mPlyFile, PLY_ASCII);
							mPlyValue.push_back (my_value_t(r6x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_VALUE ;
							mBody[i][k][j] = ix ;
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_VAR32)
						{
							INDEX ix = (INDEX) mPlyIndex.size ();
							const auto r7x = ply_read_value<int32_t>(mPlyFile, PLY_ASCII);
							mPlyIndex.push_back (my_index_t (r7x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_INDEX ;
							mBody[i][k][j] = ix ;
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_VAR64) 
						{
							INDEX ix = (INDEX) mPlyIndex.size() ;
							const auto r8x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
							mPlyIndex.push_back (my_index_t (r8x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_INDEX ;
							mBody[i][k][j] = ix ;
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_BYTE) 
						{
							INDEX ix = (INDEX) mPlyByte.size ();
							const auto r9x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
							assert (r9x >= 0);
							mPlyByte.push_back (my_byte_t (r9x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE ;
							mBody[i][k][j] = ix ;
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_WORD)
						{
							INDEX ix = (INDEX) mPlyByte.size () ;
							const auto r10x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
							assert (r10x >= 0) ;
							mPlyByte.push_back (my_byte_t (r10x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE ;
							mBody[i][k][j] = ix ;
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_CHAR) 
						{							
							INDEX ix = (INDEX) mPlyByte.size () ;
							const auto r11x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
							assert (r11x >= 0) ;
							mPlyByte.push_back(my_byte_t (r11x));
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE ;
							mBody[i][k][j] = ix ;
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_DATA)
						{
							INDEX ix = (INDEX) mPlyByte.size () ;
							const auto r12x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
							assert (r12x >= 0) ;
							mPlyByte.push_back (my_byte_t (r12x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE ;
							mBody[i][k][j] = ix ;
						}

						else {}
					}

					if (r4x != PLYREADER_PROPERY_TYPE_NULL) 
					{
						LENGTH r13x = -1 ; 
						{
							if (mBodyType[i][j] == PLYREADER_BODY_TYPE_VALUE)
								r13x =  LENGTH (mPlyValue[mBody[i][k][j]]) ;
							if (mBodyType[i][j] == PLYREADER_BODY_TYPE_INDEX)
								r13x =  LENGTH (mPlyIndex[mBody[i][k][j]]) ;
							if (mBodyType[i][j] == PLYREADER_BODY_TYPE_BYTE)
								r13x = LENGTH (mPlyByte[mBody[i][k][j]]) ;
						}
						assert (r13x != -1) ;

						if (r4x == PLYREADER_PROPERY_TYPE_VAL32) 
						{
							INDEX ix = (INDEX) mPlyValueList.size ();
							mPlyValueList.push_back (vector<my_value_t> (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_VALUE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX) mPlyValueList[ix].size() ; ++t)
							{
								const auto r14x = ply_read_value<float>(mPlyFile, PLY_ASCII);
								mPlyValueList[ix][t] = my_value_t (r14x) ;
							}
						}

						else if (r4x == PLYREADER_PROPERY_TYPE_VAL64)
						{
							INDEX ix = (INDEX)mPlyValueList.size () ;
							mPlyValueList.push_back(vector<my_value_t> (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_VALUE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX) mPlyValueList[ix].size () ; ++t) {
								const auto r15x = ply_read_value<double>(mPlyFile, PLY_ASCII); ;
								mPlyValueList[ix][t] = my_value_t (r15x) ;
							}
						}

						else if (r4x == PLYREADER_PROPERY_TYPE_VAR32)
						{
							INDEX ix = (INDEX) mPlyIndexList.size () ;
							mPlyIndexList.push_back (vector<my_index_t> (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_INDEX_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyIndexList[ix].size () ; ++t) {
								const auto r16x = ply_read_value<int32_t>(mPlyFile, PLY_ASCII);
								mPlyIndexList[ix][t] = my_index_t (r16x) ;
							}
						}

						else if (r4x == PLYREADER_PROPERY_TYPE_VAR64)
						{
							INDEX ix = (INDEX) mPlyIndexList.size () ;
							mPlyIndexList.push_back (vector<my_index_t> (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_INDEX_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyIndexList[ix].size () ; ++t) {
								const auto r17x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
								mPlyIndexList[ix][t] = my_index_t (r17x) ;
							}
						}

						else if (r4x == PLYREADER_PROPERY_TYPE_BYTE)
						{
							INDEX ix = (INDEX) mPlyByteList.size () ;
							mPlyByteList.push_back (vector<my_byte_t> (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyByteList[ix].size () ; ++t) {
								const auto r18x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
								assert (r18x >= 0) ;
								mPlyByteList[ix][t] = my_byte_t (r18x) ;
							}
						}

						else if (r4x == PLYREADER_PROPERY_TYPE_WORD)
						{
							INDEX ix = (INDEX) mPlyByteList.size () ;
							mPlyByteList.push_back (vector<my_byte_t> (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyByteList[ix].size() ; ++t) {
								const auto r19x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
								assert (r19x >= 0) ;
								mPlyByteList[ix][t] = my_byte_t (r19x) ;
							}
						}

						else if (r4x == PLYREADER_PROPERY_TYPE_CHAR)
						{
							INDEX ix = (INDEX) mPlyByteList.size () ;
							mPlyByteList.push_back (vector<my_byte_t> (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyByteList[ix].size() ; ++t) {
								const auto r20x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
								assert (r20x >= 0) ;
								mPlyByteList[ix][t] = my_byte_t (r20x) ;
							}
						}

						else if (r4x == PLYREADER_PROPERY_TYPE_DATA)
						{
							INDEX ix = (INDEX) mPlyByteList.size () ;
							mPlyByteList.push_back (vector<my_byte_t> (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyByteList[ix].size () ; ++t) {
								const auto r21x = ply_read_value<int64_t>(mPlyFile, PLY_ASCII);
								assert (r21x >= 0) ;
								mPlyByteList[ix][t] = my_byte_t (r21x) ;
							}
						}

						else {}
					}
				}
			}
		}
	}

	void read_body_binary () {

		mBody = vector<vector<vector<INDEX>>> (mHeader.mElementList.size ()) ;
		mBodyType = vector<vector<FLAG>> (mHeader.mElementList.size ()) ;

		for (INDEX i = 0 ; i < (INDEX)mHeader.mElementList.size () ; ++i) 
		{
			mBody[i] = vector<vector<INDEX>> (mHeader.mElementList[i].mSize) ;
			mBodyType[i] = vector<FLAG> (mHeader.mElementList[i].mPropertyList.size ()) ;

			for (INDEX k = 0 ; k < (INDEX)mHeader.mElementList[i].mSize ; ++k) 
			{
				mBody[i][k] = vector<INDEX> (mHeader.mElementList[i].mPropertyList.size ()) ;

				for (INDEX j = 0 ; j < (INDEX)mHeader.mElementList[i].mPropertyList.size () ; ++j) 
				{
					const auto r2x = mHeader.mElementList[i].mPropertyList[j].mType ;
					const auto r3x = mHeader.mElementList[i].mPropertyList[j].mListType ;

					if (true)
					{
						if (r2x == PLYREADER_PROPERY_TYPE_VAL32)
						{
							INDEX ix = (INDEX) mPlyValue.size () ;
							const auto r5x = ply_read_value<VAL32>(mPlyFile, mBitwiseReverseFlag);
							mPlyValue.push_back (my_value_t (r5x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_VALUE ;
							mBody[i][k][j] = ix ;
						}

						else if (r2x == PLYREADER_PROPERY_TYPE_VAL64) 
						{
							INDEX ix = (INDEX) mPlyValue.size () ;
							const auto r7x = ply_read_value<VAL64>(mPlyFile, mBitwiseReverseFlag);
							mPlyValue.push_back (my_value_t (r7x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_VALUE ;
							mBody[i][k][j] = ix ;
						}
				
						else if (r2x == PLYREADER_PROPERY_TYPE_VAR32)
						{
							INDEX ix = (INDEX) mPlyIndex.size () ;
							const auto r9x = ply_read_value<VAR32>(mPlyFile, mBitwiseReverseFlag); ;
							mPlyIndex.push_back (my_index_t (r9x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_INDEX ;
							mBody[i][k][j] = ix ;
						}

						else if (r2x == PLYREADER_PROPERY_TYPE_VAR64)
						{
							INDEX ix = (INDEX) mPlyIndex.size () ;
							const auto r11x = ply_read_value<VAR64>(mPlyFile, mBitwiseReverseFlag);
							mPlyIndex.push_back (my_index_t (r11x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_INDEX ;
							mBody[i][k][j] = ix ;
						}

						else if (r2x == PLYREADER_PROPERY_TYPE_BYTE)
						{
							INDEX ix = (INDEX) mPlyByte.size () ;
							const auto r13x = ply_read_value<BYTE>(mPlyFile, mBitwiseReverseFlag);
							mPlyByte.push_back (my_byte_t (r13x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE ;
							mBody[i][k][j] = ix ;
						}

						else if (r2x == PLYREADER_PROPERY_TYPE_WORD)
						{
							INDEX ix = (INDEX)mPlyByte.size () ;
							const auto r15x = ply_read_value<WORD>(mPlyFile, mBitwiseReverseFlag);
							mPlyByte.push_back (my_byte_t (r15x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE ;
							mBody[i][k][j] = ix ;
						}

						else if (r2x == PLYREADER_PROPERY_TYPE_CHAR)
						{
							INDEX ix = (INDEX) mPlyByte.size () ;
							const auto r17x =ply_read_value<CHAR>(mPlyFile, mBitwiseReverseFlag);
							mPlyByte.push_back (my_byte_t (r17x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE ;
							mBody[i][k][j] = ix ;
						}

						else if (r2x == PLYREADER_PROPERY_TYPE_DATA)
						{
							INDEX ix = (INDEX) mPlyByte.size () ;
							const auto r19x = ply_read_value<DATA>(mPlyFile, mBitwiseReverseFlag);
							mPlyByte.push_back (my_byte_t (r19x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE ;
							mBody[i][k][j] = ix ;
						}

						else {}
					}

					if (r3x != PLYREADER_PROPERY_TYPE_NULL)
					{
						LENGTH r20x = -1 ;
						{
							if (mBodyType[i][j] == PLYREADER_BODY_TYPE_VALUE)
								r20x = LENGTH (mPlyValue[mBody[i][k][j]]) ;
							if (mBodyType[i][j] == PLYREADER_BODY_TYPE_INDEX)
								r20x = LENGTH (mPlyIndex[mBody[i][k][j]]) ;
							if (mBodyType[i][j] == PLYREADER_BODY_TYPE_BYTE)
								r20x = LENGTH (mPlyByte[mBody[i][k][j]]) ;
						}
						assert (r20x != -1) ;

						if (r3x == PLYREADER_PROPERY_TYPE_VAL32)
						{
							// ply_read_value<VAR64>(mPlyFile, mBitwiseReverseFlag);
							INDEX ix = (INDEX) mPlyValueList.size () ;
							mPlyValueList.push_back (vector<my_value_t> (r20x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_VALUE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyValueList[ix].size () ; ++t )
							{
								const auto r22x = ply_read_value<VAL32>(mPlyFile, mBitwiseReverseFlag);
								mPlyValueList[ix][t] = my_value_t (r22x) ;
							}
						}
						
						else if (r3x == PLYREADER_PROPERY_TYPE_VAL64)
						{
							INDEX ix = (INDEX)mPlyValueList.size () ;
							mPlyValueList.push_back (vector<my_value_t> (r20x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_VALUE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyValueList[ix].size () ; ++t) {

								const auto r24x = ply_read_value<VAL64>(mPlyFile, mBitwiseReverseFlag);
								mPlyValueList[ix][t] = my_value_t (r24x) ;
							}
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_VAR32)
						{
							INDEX ix = (INDEX)mPlyIndexList.size () ;
							mPlyIndexList.push_back (vector<my_index_t> (r20x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_INDEX_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyIndexList[ix].size () ; ++t) {
								const auto r26x = ply_read_value<VAR32>(mPlyFile, mBitwiseReverseFlag);
								mPlyIndexList[ix][t] = my_index_t (r26x) ;
							}
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_VAR64)
						{
							INDEX ix = (INDEX)mPlyIndexList.size () ;
							mPlyIndexList.push_back (vector<my_index_t> (r20x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_INDEX_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX) mPlyIndexList[ix].size () ; ++t) {
								const auto r28x = ply_read_value<VAR64>(mPlyFile, mBitwiseReverseFlag);
								mPlyIndexList[ix][t] = my_index_t (r28x) ;
							}
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_BYTE)
						{
							INDEX ix = (INDEX)mPlyByteList.size () ;
							mPlyByteList.push_back (vector<my_byte_t> (r20x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyByteList[ix].size () ; ++t) {
								const auto r30x = ply_read_value<BYTE>(mPlyFile, mBitwiseReverseFlag);
								mPlyByteList[ix][t] = my_byte_t (r30x) ;
							}
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_WORD)
						{
							INDEX ix = (INDEX) mPlyByteList.size () ;
							mPlyByteList.push_back (vector<my_byte_t> (r20x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX)mPlyByteList[ix].size () ; ++t) {
								const auto r32x = ply_read_value<WORD>(mPlyFile, mBitwiseReverseFlag);
								mPlyByteList[ix][t] = my_byte_t (r32x) ;
							}
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_CHAR)
						{
							INDEX ix = (INDEX)mPlyByteList.size () ;
							mPlyByteList.push_back (vector<my_byte_t> (r20x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX) mPlyByteList[ix].size () ; ++t) {
								const auto r34x = ply_read_value<CHAR>(mPlyFile, mBitwiseReverseFlag);
								mPlyByteList[ix][t] = my_byte_t (r34x) ;
							}
						}

						else if (r3x == PLYREADER_PROPERY_TYPE_DATA)
						{
							INDEX ix = (INDEX)mPlyByteList.size () ;
							mPlyByteList.push_back (vector<my_byte_t> (r20x)) ;
							mBodyType[i][j] = PLYREADER_BODY_TYPE_BYTE_LIST ;
							mBody[i][k][j] = ix ;

							for (INDEX t = 0 ; t < (INDEX) mPlyByteList[ix].size () ; ++t) {
								const auto r36x = ply_read_value<DATA>(mPlyFile, mBitwiseReverseFlag);
								mPlyByteList[ix][t] = my_byte_t (r36x) ;
							}
						}

						else {}
					}
				}
			}
		}
	}
	
} ;


void PlyReader::check_avaliable (const my_holder_t &pointer) {
	assert (pointer != nullptr) ;
}

PlyReader::my_holder_t PlyReader::create (const my_string_t &file) {
	return std::make_shared<Implement> (file) ;
}

};
