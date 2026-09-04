#pragma once 

class Evaluator 
{
	Evaluator() = default;
	~Evaluator() = default;

	Evaluator(Evaluator&) 	= delete;
	Evaluator(Evaluator&&) 	= delete;

	Evaluator& operator=(Evaluator&) 	= delete;
	Evaluator& operator=(Evaluator&&) 	= delete;
}
