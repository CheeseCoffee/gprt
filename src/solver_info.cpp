/*
 * solver_info.cpp
 *
 *  Created on: 04 апр. 2014 г.
 *      Author: kisame
 */

#include "solver_info.h"

#include "impulse.h"
#include "options.h"
#include "gas.h"

SolverInfo::SolverInfo()
: m_pImpulse(new Impulse),
  m_pOptions(new Options)
{
	// TODO Auto-generated constructor stub
}

SolverInfo::~SolverInfo() {
	// TODO Auto-generated destructor stub
}
