/* Demo_03_Model.cpp */

#include <stdio.h>
#include <vector>
#include <boost/mpi.hpp>
#include "repast_hpc/AgentId.h"
#include "repast_hpc/RepastProcess.h"
#include "repast_hpc/Utilities.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/initialize_random.h"
#include "repast_hpc/SVDataSetBuilder.h"
#include "repast_hpc/Point.h"
#include "repast_hpc/Random.h"

#include "Model.h"


AgentPackageProvider::AgentPackageProvider(repast::SharedContext<Agent>* agentPtr): agents(agentPtr){ }


void AgentPackageProvider::providePackage(Agent * agent, std::vector<AgentPackage>& out){
	repast::AgentId id = agent->getId();
	AgentPackage package(id.id(), id.startingRank(), id.agentType(), id.currentRank(), agent->getType(), agent->getSatisfiedStatus());
	out.push_back(package);
}


void AgentPackageProvider::provideContent(repast::AgentRequest req, std::vector<AgentPackage>& out){
	std::vector<repast::AgentId> ids = req.requestedAgents();
	for(size_t i = 0; i < ids.size(); i++){
		providePackage(agents->getAgent(ids[i]), out);
	}
}




AgentPackageReceiver::AgentPackageReceiver(repast::SharedContext<Agent>* agentPtr): agents(agentPtr){}


Agent * AgentPackageReceiver::createAgent(AgentPackage package){
	repast::AgentId id(package.id, package.rank, package.type, package.currentRank);
	return new Agent(id, package.agentType, package.isSatisfied);
}


void AgentPackageReceiver::updateAgent(AgentPackage package){
	repast::AgentId id(package.id, package.rank, package.type);
	Agent * agent = agents->getAgent(id);
	agent->set(package.currentRank, package.agentType, package.isSatisfied);
}


SchellingModel::SchellingModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm): context(comm){
	props = new repast::Properties(propsFile, argc, argv, comm);
	stopAt = repast::strToInt(props->getProperty("stop.at"));
	countOfAgents = repast::strToInt(props->getProperty("count.of.agents"));
	boardSize = repast::strToInt(props->getProperty("board.size"));
	initializeRandom(*props, comm);
repast::Point<double> origin(0,0);
	repast::Point<double> extent(boardSize,boardSize);
	repast::GridDimensions gd(origin, extent);

	int procX = repast::strToInt(props->getProperty("proc.per.x"));
	int procY = repast::strToInt(props->getProperty("proc.per.y"));
	int bufferSize = repast::strToInt(props->getProperty("grid.buffer"));


	std::vector<int> processDims;
	processDims.push_back(procX);
	processDims.push_back(procY);
	discreteSpace = new repast::SharedDiscreteSpace<Agent, repast::StrictBorders, repast::SimpleAdder<Agent> >("AgentDiscreteSpace", gd, processDims, bufferSize, comm);
	
	printf("Rank %d. Bounds (global): (%.1f,%.1f) & (%.1f,%.1f). Dimensions (local): (%.1f,%.1f) & (%.1f,%.1f).\n",
		repast::RepastProcess::instance()->rank(),
		discreteSpace->bounds().origin().getX(),  discreteSpace->bounds().origin().getY(),
		discreteSpace->bounds().extents().getX(), discreteSpace->bounds().extents().getY(),
		discreteSpace->dimensions().origin().getX(),  discreteSpace->dimensions().origin().getY(),
		discreteSpace->dimensions().extents().getX(), discreteSpace->dimensions().extents().getY()
	);

	context.addProjection(discreteSpace);

	provider = new AgentPackageProvider(&context);
	receiver = new AgentPackageReceiver(&context);


}

SchellingModel::~SchellingModel(){
	delete props;
	
	delete provider;
	
	delete receiver;

}



	void SchellingModel::initAgents()
	{
	int rank = repast::RepastProcess::instance()->rank();
	//repast::IntUniformGenerator gen = repast::Random::instance()->createUniIntGenerator(1, boardSize);
	int xMin = ceil(discreteSpace->dimensions().origin().getX());
	int xMax = floor(discreteSpace->dimensions().origin().getX() + discreteSpace->dimensions().extents().getX() - 0.00001);
	int yMin = ceil(discreteSpace->dimensions().origin().getY());
	int yMax = floor(discreteSpace->dimensions().origin().getY() + discreteSpace->dimensions().extents().getY() - 0.00001);
	//printf("Init: Rank %d. (%d,%d)->(%d,%d).\n", repast::RepastProcess::instance()->rank(), xMin, yMin, xMax, yMax);
	repast::IntUniformGenerator genX = repast::Random::instance()->createUniIntGenerator(xMin, xMax);
	repast::IntUniformGenerator genY = repast::Random::instance()->createUniIntGenerator(yMin, yMax);


	double threshold = repast::strToDouble(props->getProperty("threshold"));
	int x, y;
		for(int i = 0; i < countOfAgents; i++) 
		{
		if (i==0 && rank==0) {
			x=1; y=2;
		} else if (i==0 && rank==1) {
			x=2; y=3;
		} else if (i==0 && rank==2) {
			x=4; y=2;
		} else if (i==0 && rank==3) {
			x=5; y=5;
		}
		repast::Point<int> initialLocation(x, y);
		
		repast::AgentId id(i, rank, 0);
		id.currentRank(rank);
		Agent* agent = new Agent(id, 0, threshold);
             agent->set(rank, 0, false);
		context.addAgent(agent);
		discreteSpace->moveTo(id, initialLocation);


		std::vector<int> agentLoc;
		discreteSpace->getLocation(id, agentLoc);
		//printf("Init: Agent %d,%d,%d - at (%d,%d).\n", id.id(), id.startingRank(), id.currentRank(), agentLoc[0], agentLoc[1]);
	}
}



	/*
	int rank = repast::RepastProcess::instance()->rank();
	repast::IntUniformGenerator gen = repast::Random::instance()->createUniIntGenerator(1, boardSize); //random from 1 to boardSize
	int countType0 = countOfAgents/2; //half type 0
	int countType1 = countOfAgents - countType0; // the rest type 1
	double threshold = repast::strToDouble(props->getProperty("threshold"));
	for (int i = 0; i < countOfAgents; i++){
		//random a location until an empty one is found (not the most efficient)
		int xRand, yRand;
		std::vector<Agent*> agentList;
		do {
			agentList.clear();
			xRand = gen.next(); yRand = gen.next();
			discreteSpace->getObjectsAt(repast::Point<int>(xRand, yRand), agentList);
		} while (agentList.size() != 0);


		//create agent, assign type, move the agent to the randomised location
		repast::Point<int> initialLocation(xRand, yRand);
		repast::AgentId id(i, rank, 0);
		id.currentRank(rank);
		int type;
		if (countType0 > 0) {
			type = 0;
			countType0--;
		} else {
			type = 1;
			countType1--;
		}
		Agent* agent = new Agent(id, type, threshold);
		context.addAgent(agent);
		discreteSpace->moveTo(id, initialLocation);
	}
	*/


/*void SchellingModel::doPerTick(){
	
//calculate avg satisfaction
	double avgSatisfied = 0;
	std::vector<Agent*> agents;
	context.selectAgents(repast::SharedContext<Agent>::LOCAL, countOfAgents, agents);
	std::vector<Agent*>::iterator it = agents.begin();
	while(it != agents.end()){
		(*it)->updateStatus(&context, discreteSpace);
		avgSatisfied += (*it)->getSatisfiedStatus();
		it++;
	}
	avgSatisfied /= countOfAgents;



	//agents move to a random location if unsatisfied
	it = agents.begin();
	while(it != agents.end()){
		if (!(*it)->getSatisfiedStatus())
			(*it)->move(discreteSpace);
		it++;
	}
	

}*/

void SchellingModel::initSchedule (repast::ScheduleRunner& runner)
{
	runner.scheduleEvent(1, 1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<SchellingModel> (this, &SchellingModel::doPerTick)));
	runner.scheduleStop(stopAt);
}
