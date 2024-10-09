#ifndef MODEL
#define MODEL

#include <boost/mpi.hpp>
#include "repast_hpc/Schedule.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/SharedContext.h"
#include "repast_hpc/SharedDiscreteSpace.h"
#include "repast_hpc/GridComponents.h"
#include "repast_hpc/AgentRequest.h"


#include "Agent.h"


/* Agent Package Provider */
class AgentPackageProvider {
	
private:
	repast::SharedContext<Agent>* agents;
	
public:
	

	AgentPackageProvider(repast::SharedContext<Agent>* agentPtr);
	
	void providePackage(Agent * agent, std::vector<AgentPackage>& out);
	
	void provideContent(repast::AgentRequest req, std::vector<AgentPackage>& out);


};


/* Agent Package Receiver */
class AgentPackageReceiver {
	
private:
	repast::SharedContext<Agent>* agents;
	
public:
	
	AgentPackageReceiver(repast::SharedContext<Agent>* agentPtr);
	
	Agent * createAgent(AgentPackage package);
	
	void updateAgent(AgentPackage package);
	
};



class SchellingModel{
private:
	int stopAt;
	int countOfAgents;
	int boardSize;
	repast::Properties* props;
	repast::SharedContext<Agent> context;
	repast::SharedDiscreteSpace<Agent, repast::StrictBorders, repast::SimpleAdder<Agent> >* discreteSpace;
	
	AgentPackageProvider* provider;
	
	AgentPackageReceiver* receiver;

public:
	SchellingModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm);
	~SchellingModel();
	void initAgents();
	void initSchedule(repast::ScheduleRunner& runner);
	void doPerTick();
};

#endif
