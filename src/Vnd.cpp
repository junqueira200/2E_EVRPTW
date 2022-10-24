

#include "Vnd.h"
#include "mersenne-twister.h"
#include "LocalSearch.h"

using namespace NS_vnd;
using namespace NS_LocalSearch;


void NS_vnd::rvnd(Solucao &solution, Instance &instance, const float beta, std::vector<MvValor> &vetMvValor)
{

    static int vetMv[NUM_MV];

    for(int i=0; i < NUM_MV; ++i)
    {
        vetMv[i] = rand_u32()%NUM_MV;
        bool invalido = true;

        while(invalido)
        {
            int j=0;
            for(j = 0; j < i; ++j)
            {
                if(vetMv[j] == vetMv[i])
                {
                    vetMv[i] = (vetMv[i]+1)%NUM_MV;
                    j=0;
                    break;
                }
            }

            if(j==i)
                invalido = false;

        }
    }


    EvRoute evRouteAux(1, instance.getFirstEvIndex(), instance.evRouteSizeMax, instance);
    EvRoute evRouteAux1(1, instance.getFirstEvIndex(), instance.evRouteSizeMax, instance);

    int i = 0;

    try
    {

        bool valEsp = false;
        double valOrig = 0.0;
        while(i < NUM_MV)
        {
            if(i == 0)
                valOrig = solution.distancia;

            bool aplicacao = false;
            double val = solution.distancia;
            //cout << "\t\tMV: " << vetMv[i] << "\n";

            switch(vetMv[i])
            {
                case MV_EV_SHIFIT_INTRA_ROTA:
                    aplicacao = mvEvShifitIntraRota(solution, instance, evRouteAux, SELECAO_PRIMEIRO);
                    break;

                case MV_EV_SWAP_INTRA_ROTA:
                    aplicacao = mvEvSwapIntraRota(solution, instance, evRouteAux);
                    break;

                case MV_EV_2OPT:
                    aplicacao = mvEv2opt(solution, instance, evRouteAux);
                    break;

                case MV_EV_SHIFIT_INTER_ROTAS_INTRA_SAT:
                    aplicacao = mvEvShifitInterRotasIntraSat(solution, instance, evRouteAux, evRouteAux1);
                    break;

                case MV_EV_SHIFIT_INTER_ROTAS_INTER_SAT:
                    aplicacao = mvEvShifitInterRotasInterSats(solution, instance, evRouteAux, evRouteAux1, beta);
                    break;


                default:
                    cout << "ERRO: MV(" << i << ") NAO EXISTE\n";
                    throw "ERRO";
                    break;

            }

            if(aplicacao)
            {
                vetMvValor[vetMv[i]].add(valOrig, solution.distancia);

                string erro;
                if(!solution.checkSolution(erro, instance))
                {
                    PRINT_DEBUG("", "ERRO. MV: " << i << "\n");
                    cout << erro << "\n\n";
                    solution.print(instance);

                    throw "ERRO";
                }

                i = 0;
            } else
                i += 1;

            //cout<<"#############################################################\n\n";

        }

    }
    catch(const char *erro)
    {

        PRINT_DEBUG("", "ERRO. MV: " << i << "\n");
        cout << erro << "\n\n";
        solution.print(instance);

        throw "ERRO";
    }

    //cout<<"//////////////////////////////////////////////////////////////////\n\n";

}