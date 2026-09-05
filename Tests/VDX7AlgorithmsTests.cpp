#include "VDX7Algorithms.h"
#include <fstream>
#include <iterator>
#include <regex>
#include <iostream>
#include <cstdlib>
#include <vector>

void require(bool v,const char* message)
{ if (!v) { std::cerr << message << '\n'; std::exit(1); } }

int main(int argc,char** argv)
{
    using namespace VDX7Algorithms;
    require(get(5).carriers==21 && get(16).carriers==1 && get(32).carriers==63,
            "Manual reference: algorithms 5, 16 and 32 carriers");
    require(get(4).feedbackFrom==3 && get(4).feedbackTo==5,"Algorithm 4: OP4 to OP6 feedback");
    require(get(6).feedbackFrom==4 && get(6).feedbackTo==5,"Algorithm 6: OP5 to OP6 feedback");
    for (const auto& graph:graphs)
    {
        int outgoing=0;
        for (int target=0;target<6;++target)
        {
            require((graph.inputs[target] & ((1<<(target+1))-1))==0,"Forward graph must be acyclic");
            outgoing|=graph.inputs[target];
        }
        require((outgoing ^ 63)==graph.carriers,"Every terminal operator connects to output");
        auto depth=levels(graph);
        for (int d:depth) require(d>=0 && d<=3,"Node layout level bounds");
    }
    // Verify all 32 diagrams against the exact core fetched for this build.
    require(argc==2,"OPS.h path required");
    std::ifstream file(argv[1]);
    require(bool(file),"Cannot read emulation-core algorithm table");
    std::string source((std::istreambuf_iterator<char>(file)),{});
    std::regex entry(R"(\{SEL([0-5]),([01]),([01]),([01]),([0-5])\})");
    std::vector<std::array<int,4>> instructions;
    for (std::sregex_iterator i(source.begin(),source.end(),entry),end;i!=end;++i)
        instructions.push_back({std::stoi((*i)[1]),std::stoi((*i)[2]),std::stoi((*i)[3]),std::stoi((*i)[4])});
    require(instructions.size()==192,"Unexpected core ROM table format");
    for (int a=0;a<32;++a)
    {
        int accumulator=0,modulation=0,feedbackRegister=0;
        bool feedbackRoute=false;
        for (int cycle=0;cycle<3;++cycle)
            for (int block=0;block<6;++block)
            {
                int op=5-block;
                if (cycle==2)
                {
                    if (feedbackRoute)
                        require(op==graphs[a].feedbackTo && modulation==(1<<graphs[a].feedbackFrom),"Core feedback mismatch");
                    else require(modulation==graphs[a].inputs[op],"Core modulation-edge mismatch");
                }
                const auto ins=instructions[a*6+block];
                int signal=1<<op;
                int sum=(ins[2]?accumulator:0)|(ins[3]?signal:0);
                int sources[]={0,signal,sum,accumulator,feedbackRegister,feedbackRegister};
                modulation=sources[ins[0]];
                feedbackRoute=ins[0]==5;
                accumulator=sum;
                if (ins[1]) feedbackRegister=signal;
            }
        require(accumulator==graphs[a].carriers,"Core output-carrier mismatch");
    }
    std::cout << "PASS: all 32 diagrams match core routing, carriers and feedback; manual reference cases and layout depths\n";
}
