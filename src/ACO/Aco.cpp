/* ****************************************
 * ****************************************
 *  Nome:    Igor de Andrade Junqueira
 *  Data:    21/07/22
 *  Arquivo: Aco.cpp
 * ****************************************
 * ****************************************/

#include "Aco.h"
#include "Instance.h"
#include "../greedyAlgorithm.h"
#include "../mersenne-twister.h"
#include <iostream>
#include <boost/numeric/ublas/matrix.hpp>


using namespace std;
using namespace boost::numeric;

void N_Aco::aco(Instance &instance, AcoParametros &acoPar, AcoEstatisticas &acoEst, const vector<int8_t> &clientes, const int sateliteId, Satelite &satBest)
{

    Solucao solucao(instance);
    bool construtivo = GreedyAlgNS::secondEchelonGreedy(solucao, instance, acoPar.alfaConst);

    if(!construtivo)
    {
        for (int i = 0; i < acoPar.numItMaxHeur; ++i)
        {
            solucao = Solucao(instance);
            construtivo = GreedyAlgNS::secondEchelonGreedy(solucao, instance, acoPar.alfaConst);

            if(construtivo)
                break;
        }
    }

    ublas::matrix<double> matFeromonio(instance.numNos, instance.numNos, acoPar.feromonioInicial);
    Ant antBest(instance, sateliteId, true);
    vector<Proximo> vetProximo(1+instance.numClients+instance.numRechargingS);

    for(int iteracoes = 0; iteracoes < acoPar.numIteracoes; ++iteracoes)
    {

        std::vector<Ant> vetAnt(acoPar.numAnts, Ant(instance, sateliteId));

        for(Ant &ant:vetAnt)
        {
            int rotaEv = -1;

            while(existeClienteNaoVisitado(ant, instance))
            {
                // Cria uma nova rota
                rotaEv += 1;

                if(rotaEv < instance.getN_Evs())
                {
                    EvRoute &evRoute = ant.satelite.vetEvRoute[rotaEv];
                    evRoute.routeSize = 1;

                    int pos             = 0;
                    double multSoma     = 0.0;
                    double multMax      = -1.0;
                    int multMaxIndice   = -1;
                    int proxVetProximo  = 0;

                    auto atualizaVetProx = [&](int clienteJ)
                    {
                        double temp = vetProximo[proxVetProximo].atualiza(sateliteId, instance.getDistance(evRoute[pos].cliente, clienteJ),
                                                                          matFeromonio(evRoute[pos].cliente, clienteJ), acoPar);
                        if(temp > multMax)
                        {
                            multMax         = temp;
                            multMaxIndice   = proxVetProximo;
                        }

                        multSoma += temp;
                        proxVetProximo += 1;

                    };

                    // Escolhe a proxima aresta
                    do
                    {
                        const int clienteI = evRoute[pos].cliente;

                        proxVetProximo = 0;

                        // satelite
                        if(clienteJValido(instance, clienteI, sateliteId, evRoute[pos].bateriaRestante, ant.vetNosAtend, sateliteId))
                            atualizaVetProx(sateliteId);


                        // Clientes
                       for(int j=instance.getFirstClientIndex(); j < instance.getEndClientIndex(); ++j)
                       {
                           // Verifica Capacidade de carga do EV
                           if(j != clienteI && (evRoute.demanda+instance.getDemand(j)) <= instance.getEvCap(rotaEv))
                           {
                                if(clienteJValido(instance, clienteI, j, evRoute[pos].bateriaRestante, ant.vetNosAtend, sateliteId))
                                    atualizaVetProx(j);
                           }
                       }

                       // Estacoes de Recarga
                       for(int j=instance.getFirstRechargingSIndex(); j <= instance.getEndRechargingSIndex(); ++j)
                       {
                           if(clienteJValido(instance, clienteI, j, evRoute[pos].bateriaRestante, ant.vetNosAtend, sateliteId))
                               atualizaVetProx(j);
                       }

                       if(proxVetProximo != 0)
                       {
                           int proxClienteInd = 0;
                           static const int q0 = int(acoPar.q0*100);

                           if((rand_u32()%101) <= q0)
                           {
                                proxClienteInd = multMaxIndice;
                           }
                           else
                           {
                               double prob = 0.0;
                               for(int i=0; i < proxVetProximo; ++i)
                               {
                                   prob = (vetProximo[i].ferom_x_dist/multSoma) * 100;
                                   if((rand_u32()%101) <= int(prob))
                                   {
                                        proxClienteInd = i;
                                        break;
                                   }
                               }
                           }

                           // Cliente j foi escolhido
                           pos += 1;

                           // Atualiza evRoute
                           atualizaClienteJ(evRoute, pos, vetProximo[proxClienteInd].cliente, instance);


                       }
                       else
                       {
                           ant.viavel = false;
                           break;
                       }


                    }while(evRoute[pos].cliente != sateliteId);

                }
                else
                    break;

            }

            if(!existeClienteNaoVisitado(ant, instance))
                ant.viavel = true;
        }
    }
}

void N_Aco::atualizaClienteJ(EvRoute &evRoute, const int pos, const int clienteJ, Instance &instance)
{

    const double dist_i_j = instance.getDistance(evRoute[pos-1].cliente, clienteJ);
    evRoute[pos].cliente = clienteJ;
    evRoute.routeSize = pos+1;
    evRoute[pos].tempoCheg = evRoute[pos-1].tempoSaida+dist_i_j;

    double bat = evRoute[pos].bateriaRestante-dist_i_j;


    if(bat < -TOLERANCIA_BATERIA)
    {
        string strRota;
        evRoute.print(strRota, instance, false);
        PRINT_DEBUG("", "ERRO, Deveria ser possivel chegar no cliente "<<clienteJ<<"; bat: "<<evRoute[pos].bateriaRestante<<"\nRota: "<<strRota);
        throw "ERRO";
    }

    if(instance.isRechargingStation(clienteJ))
    {
        double dif = instance.getEvBattery(evRoute.idRota) - (evRoute[pos].bateriaRestante-dist_i_j);
        evRoute[pos].tempoSaida = evRoute[pos].tempoCheg+instance.vectVeiculo[evRoute.idRota].taxaRecarga*dif;
        bat = instance.getEvBattery(evRoute.idRota);
    }
    else
    {
        evRoute[pos].tempoSaida = evRoute[pos].tempoCheg+instance.vectCliente[clienteJ].tempoServico;
    }


    evRoute[pos].bateriaRestante = bat;
    evRoute.distancia += dist_i_j;
    evRoute.demanda += instance.vectCliente[clienteJ].demanda;

}

bool N_Aco::clienteJValido(Instance &instancia, const int i, const int j, const double bat, const vector<int8_t> &vetNosAtend, const int sat)
{
    if(i == j || vetNosAtend[j] == 0)
        return false;

    const double dist_i_j = instancia.getDistance(i,j);
    double batTemp = bat - dist_i_j;


    if((bat-dist_i_j) >= -TOLERANCIA_BATERIA)
    {

        // Verifica se j eh estacao de recarga
        if(instancia.isRechargingStation(j))
        {
            if(vetNosAtend[j] < instancia.numUtilEstacao)
                return true;
            else
                return false;
        }

        // Verifica se eh possivel retornar ao satelite
        if((batTemp-instancia.getDistance(j, sat)) >= -TOLERANCIA_BATERIA)
            return true;

        // Percorre as estacoes de recarga
        for(int es=instancia.getFirstRechargingSIndex(); es <= instancia.getEndRechargingSIndex(); ++es)
        {
            if(vetNosAtend[es] < instancia.numUtilEstacao)
            {
                if((batTemp - instancia.getDistance(j, es)) >= -TOLERANCIA_BATERIA)
                    return true;
            }
        }

        // Nao eh possivel retornar ao deposito ou chegar no satelite
        return false;
    }
    else
        return false;

}


void N_Aco::atualizaFeromonio(ublas::matrix<double> &matFeromonio, Instance &instancia, const AcoParametros &acoParam, const Ant &antBest, const double feromMin, const double feromMax)
{
    double inc = 1.0/getDistSat(antBest.satelite);
    evaporaFeromonio(matFeromonio, {antBest.satelite.sateliteId}, instancia, acoParam, feromMin);

    // Percorre a solucao para add feromonio 1/dist
    for(const EvRoute &evRoute:antBest.satelite.vetEvRoute)
    {
        if(evRoute.routeSize > 2)
        {
            for(int i=0; i < (evRoute.routeSize-1); ++i)
            {
                const double feromTemp = matFeromonio(evRoute.route[i].cliente, evRoute.route[i+1].cliente);
                matFeromonio(evRoute.route[i].cliente, evRoute.route[i+1].cliente) = min((feromTemp+inc), feromMax);
            }
        }
    }
}

void N_Aco::evaporaFeromonio(ublas::matrix<double> &matFeromonio, const vector<int> &vetSat, Instance &instancia, const AcoParametros &acoParam, const double feromMin)
{

    static const double ro_1 = 1.0 - acoParam.ro;

    // Atualizacao das arestas (i,j), i,j clientes
    for(int i=instancia.getFirstClientIndex(); i <= instancia.getEndClientIndex(); ++i)
    {
        for(int j=instancia.getFirstClientIndex(); j <= instancia.getEndClientIndex(); ++j)
        {
            if(i==j)
                continue;

            matFeromonio(i,j) = max(matFeromonio(i,j)*ro_1, feromMin);
        }
    }

    // Atualizacoes arestas (i,j), i sat e j cliente
    for(const int s:vetSat)
    {

        for(int i=instancia.getFirstClientIndex(); i <= instancia.getEndClientIndex(); ++i)
            matFeromonio(s, i) = max(matFeromonio(s, i)*ro_1, feromMin);
    }

    // Atualizacoes arestas (i,j), i cliente e j cliente
    for(int i=instancia.getFirstClientIndex(); i <= instancia.getEndClientIndex(); ++i)
    {
        for(const int s:vetSat)
            matFeromonio(i, s) = max(matFeromonio(i, s)*ro_1, feromMin);
    }

}


