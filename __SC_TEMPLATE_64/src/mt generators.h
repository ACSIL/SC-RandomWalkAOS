#pragma once
#include<chrono>
#include<random>
 
//unsigned int rn_generator();
//unsigned char ls_generator();


//
//unsigned int rn_generator(SCStudyInterfaceRef sc)
//{
//	std::mt19937 gen;
//	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
//	std::uniform_int_distribution<int> distribution(10, 1000); //generate numbers from 60 do 3600 (min - hour)
//	return distribution(gen);
//}
//
//unsigned char ls_generator()
//{
//	std::mt19937 gen;
//	gen.seed(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
//	std::uniform_int_distribution<int> distribution(0, 1); //generate 1/0 to decide for long/short
//	return distribution(gen);
//}