#include "string_converter.hh"

std::vector<std::int16_t> string_converter::to_vector_int16(
	std::string& body)
{
	// Zapamiętaj długość stringa:
	const size_t body_size = body.size();
	// Skonwertuj stringa na wektor charów:
	std::vector<char> c_vector(body.begin(), body.end());
	// Stwórz inteligentny wskaźnik 16-bitowy ze znakiem:
	std::int16_t* s_ptr(
		reinterpret_cast<std::int16_t*>(c_vector.data())	
	);
	// Skonwertuj go na wektor liczb 16-bitowych ze znakiem:
	std::vector<std::int16_t> s_vector(s_ptr, s_ptr + (body_size / 2));
	return s_vector;
}
