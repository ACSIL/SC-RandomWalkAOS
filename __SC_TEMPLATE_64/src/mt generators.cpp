//#include<random>
//#include<chrono>
//#include "sierrachart.h"
//
//unsigned int rn_generator(SCStudyInterfaceRef sc)
//{
//	std::mt19937 gen;
//	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
//	std::uniform_int_distribution<int> distribution(sc.Input[10].GetInt(), sc.Input[11].GetInt()); //generate numbers from 60 do 3600 (min - hour)
//	return distribution(gen);
//}
//
////unsigned char ls_generator()
////{
////	std::mt19937 gen;
////	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
////	std::uniform_int_distribution<int> distribution(0, 1); //generate 1/0 to decide for long/short
////	return distribution(gen);
////}
////
