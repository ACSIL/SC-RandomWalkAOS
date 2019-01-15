#include<random>
#include<chrono>

//unsigned int rn_generator()
//{
//	std::mt19937 gen;
//	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
//	std::uniform_int_distribution<int> distribution(60, 360); //generate numbers from 60 do 3600 (min - hour)
//	return distribution(gen);
//}

//unsigned char ls_generator()
//{
//	std::mt19937 gen;
//	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
//	std::uniform_int_distribution<int> distribution(0, 100); //generate 1/0 to decide for long/short
//	return distribution(gen);
//}
