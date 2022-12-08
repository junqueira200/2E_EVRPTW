/* ****************************************
 * ****************************************
 *  Nome:    Igor de Andrade Junqueira
 *  Data:    25/11/22
 *  Arquivo: LocalSearch2.h
 * ****************************************
 * ****************************************/

#ifndef INC_2E_EVRP_LOCALSEARCH2_H
#define INC_2E_EVRP_LOCALSEARCH2_H

#define MV_INVIAVEL                 0
#define MV_VIAVEL                   1
#define MV_POS_EV_ROUTE0_INVIAVEL   2
#define MV_POS_EV_ROUTE1_INVIAVEL   3
#define MV_EV_ROUTE0_INVIAVEL       4
#define MV_EV_ROUTE1_INVIAVEL       5

#include "../Solucao.h"
#include "Instancia.h"

namespace NS_LocalSearch2
{

    bool mvEvShifit2Nos_interRotasIntraSat(Solucao &solucao, Instancia &instancia, EvRoute &evRouteAux0, EvRoute &evRouteAux1);
    bool mvEvShifit2Nos_interRotasInterSats(Solucao &solucao, Instancia &instancia, EvRoute &evRouteAux0,
                                            EvRoute &evRouteAux1, float beta);

    int cross(Instancia &instancia, EvRoute &evRoute0, int posEvRoute0, EvRoute &evRoute1, int posEvRoute1, EvRoute &evRouteAux0,
              EvRoute &evRouteAux1, double tempoSaidaSatRoute0, double tempoSaidaSatRoute1);

    void copiaCliente(const BoostC::vector<EvNo> &vet0, BoostC::vector<EvNo> &vetDest, int tam, int ini=0);
    int copiaCliente(const BoostC::vector<EvNo> &vet0, BoostC::vector<EvNo> &vetDest, int iniVet0, int fimVet0, int iniVetDest);

    template<typename Func>
    bool mvInterRotasIntraSat(Solucao &solucao, Instancia &instancia, EvRoute &evRouteAux0, EvRoute &evRouteAux1, Func func)
    {

        for(int sat = 1; sat <= instancia.getNSats(); ++sat)
        {

            for(int ev0=0; ev0 < instancia.getN_Evs(); ++ev0)
            {
                EvRoute &evRoute0 = solucao.satelites[sat].vetEvRoute[ev0];

                for(int ev1=0; ev1 < instancia.getN_Evs(); ++ev1)
                {
                    if(ev0 == ev1)
                        continue;

                    EvRoute &evRoute1 = solucao.satelites[sat].vetEvRoute[ev1];

                    // Selecionar as posicoes das rotas
                    for(int posEv0=0; posEv0 < (evRoute0.routeSize-1); ++posEv0)
                    {

                        for(int posEv1=0; posEv1 < (evRoute1.routeSize-1); ++posEv1)
                        {

                            const double distOrig    = evRoute0.distancia+evRoute1.distancia;
                            const double demandaOrig = evRoute0.demanda+evRoute1.demanda;

                            int resutado = func(instancia, evRoute0, posEv0, evRoute1, posEv1, evRouteAux0, evRouteAux1, evRoute0[0].tempoSaida, evRoute1[0].tempoSaida);


                            if(resutado == 1)
                            {
                                const double novaDist = evRouteAux0.distancia + evRouteAux1.distancia;
                                solucao.distancia += -distOrig + novaDist;
                                solucao.satelites[sat].distancia += -distOrig + novaDist;

                                evRouteAux0.satelite = evRoute0.satelite;
                                evRouteAux0.idRota   = evRoute0.idRota;


                                evRouteAux1.satelite = evRoute1.satelite;
                                evRouteAux1.idRota   = evRoute1.idRota;

                                evRoute0.copia(evRouteAux0, true, &instancia);
                                evRoute1.copia(evRouteAux1, true, &instancia);

                                evRoute0.distancia = evRouteAux0.distancia;
                                evRoute1.distancia = evRouteAux1.distancia;

                                evRoute0.atualizaParametrosRota(instancia);
                                evRoute1.atualizaParametrosRota(instancia);


//cout<<"MV UPDATE\n";

                                double novaDemanda = evRoute0.demanda + evRoute1.demanda;
                                if(novaDemanda != demandaOrig)
                                {
                                    PRINT_DEBUG("","");
                                    cout<<"ERRO! NOVA DEMANDA("<<novaDemanda<<") != DEMANDA ORIGINAL("<<demandaOrig<<")\n";
                                    throw "ERRO";
                                }
//cout<<"\n\n****************************************************\n****************************************************\n\n";
                                return true;
                            }
                            else
                            {
//cout<<"\n\n****************************************************\n****************************************************\n\n";
                            }

                        } // End for(posEvSat1)

                    } // End for(posEvSat0)

                } // End for(evSat1)

            } // End for(evSat0)

        } // End for(sat1)

        return false;
    }

}

#endif //INC_2E_EVRP_LOCALSEARCH2_H
