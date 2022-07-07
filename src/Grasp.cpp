/* ****************************************
 * ****************************************
 *  Nome:    Igor de Andrade Junqueira
 *  Data:    02/06/22
 *  Arquivo: Grasp.cpp
 * ****************************************
 * ****************************************/

#include "Grasp.h"
#include "greedyAlgorithm.h"
#include "Auxiliary.h"
#include "mersenne-twister.h"
#include "LocalSearch.h"
#include "PreProcessamento.h"

#define NUM_EST_INI 3

using namespace GreedyAlgNS;
using namespace NameS_Grasp;
using namespace NS_LocalSearch;

const float fator = 0.1;

Solucao * NameS_Grasp::grasp(Instance &instance, Parametros &parametros, Estatisticas &estat)
{
    Solucao *solBest = new Solucao(instance);
    solBest->distancia = DOUBLE_MIN;
    solBest->viavel = false;
    EvRoute evRoute(1, instance.getFirstEvIndex(), instance.getEvRouteSizeMax(), instance);

    estat.numSol = 0.0;
    estat.numIte = parametros.numIteGrasp;
    estat.distAcum = 0.0;
    estat.erro = "";

    const int tamAlfa = parametros.vetAlfa.size();

    // Solucao para inicializar reativo
    Solucao gul(instance);
    construtivo(gul, instance, 0.0, 0.0);
    const double gulCusto = getDistMaisPenalidade(gul, instance);
    double custoBest = gulCusto;

    //Vetores para o reativo
    std::vector<double> vetorProbabilidade(tamAlfa);
    std::fill(vetorProbabilidade.begin(), vetorProbabilidade.begin()+tamAlfa, 100.0/float(tamAlfa));

    std::vector<int>    vetorFrequencia(tamAlfa);
    std::fill(vetorFrequencia.begin(), vetorFrequencia.begin()+tamAlfa, 1);

    std::vector<double> solucaoAcumulada(tamAlfa);
    std::fill(solucaoAcumulada.begin(), solucaoAcumulada.begin()+tamAlfa, gulCusto);

    std::vector<double> vetorMedia(tamAlfa);
    std::fill(vetorMedia.begin(), vetorMedia.begin()+tamAlfa, 0.0);

    std::vector<double> proporcao(tamAlfa);
    std::fill(proporcao.begin(), proporcao.begin()+tamAlfa, 0.0);


    auto atualizaProb = [&]()
    {
        double somaProporcoes = 0.0;

        //Calcular média

        for(int i = 0; i < tamAlfa; ++i)
            vetorMedia[i] = solucaoAcumulada[i]/double(vetorFrequencia[i]);

        //Calcula proporção.
        for(int i = 0; i < tamAlfa; ++i)
        {
            proporcao[i] = custoBest/vetorMedia[i];
            somaProporcoes += proporcao[i];
        }

        //Calcula probabilidade
        for(int i = 0; i< tamAlfa; ++i)
            vetorProbabilidade[i] = 100.0*(proporcao[i]/somaProporcoes);

        //cout<<"vet prob: \n";
        //string vet = NS_Auxiliary::printVector(vetorProbabilidade, int64_t(vetorProbabilidade.size()));

        //cout<<vet<<"\n";

        double sum = 0.0;
        for(int i=0; i < tamAlfa; ++i)
            sum += vetorProbabilidade[i];

        //cout<<"sum: "<<sum<<"\n\n";

    };

    auto convIndClienteVet = [&](int pos)
    {
        return pos-instance.getFirstClientIndex();
    };

    // Guarda o numero de vezes que o cliente i NAO a parece na solucao
    std::vector<QuantCliente> vetQuantCliente(instance.getNClients());

    for(int i=instance.getFirstClientIndex(); i <= instance.getEndClientIndex(); ++i)
        vetQuantCliente[convIndClienteVet(i)].cliente = i;


    double somaProb = 0.0;
    int posAlfa = 0;
    int valAleatorio = 0;
    float alfa = 0;

    int addRotaClienteProbIgual = 0;
    int clienteAdd = -1;



    for(int i=0; i < parametros.numIteGrasp; ++i)
    {
        Solucao sol(instance);

        if(i == parametros.iteracoesCalProb) //&& (i%parametros.iteracoesCalProb)==0)
        {
            std::sort(vetQuantCliente.begin(), vetQuantCliente.end());

            for(int t=0; t < instance.getNClients(); ++t)
            {

                const EvRoute &evRouteAux = instance.shortestPath->vetEvRoute[t];

                vetQuantCliente[t].calculaProb(i);
                if(vetQuantCliente[t].prob == 0 && evRouteAux.routeSize > 2)
                {
                    //vetQuantCliente[t].prob = 5;
                    addRotaClienteProbIgual += 1;
                }
                else if(vetQuantCliente[t].prob >= 90 && evRouteAux.routeSize > 2)
                {
                    //vetQuantCliente[t].prob = 90;
                    //addRotaClienteProbIgual += 1;

                }
                else if(evRouteAux.routeSize <= 2)
                    vetQuantCliente[t].prob = 100;
            }

            //cout<<"IGUAL: "<<addRotaClienteProbIgual<<"\n";

            //cout<<"vetQuantCliente: ";
            NS_Auxiliary::printVectorCout(vetQuantCliente, vetQuantCliente.size());

            if(addRotaClienteProbIgual >= 2)
            {
                parametros.numMaxClie = instance.getN_Evs();
                //cout<<"num max clie: "<<parametros.numMaxClie<<"\n";
            }

            //cout<<"\n\n";

        }

        if(i >= parametros.iteracoesCalProb && parametros.iteracoesCalProb > 0)
        {
            int clientesAdd = 0;

            bool add = false;
            bool igual = false;
            int next = 0;

/*            if(addRotaClienteProbIgual > 0)
            {
                igual = true;
                next = i % addRotaClienteProbIgual;
                //cout<<"next: "<<next<<"\n";

            }*/

            int t=0;

            if(igual)
                t = next;

            if(vetQuantCliente[0].prob != 100)
            {

                while(t < instance.getNClients())
                {


                    if(vetQuantCliente[t].prob != 100)
                    {

                        if((rand_u32() % 100) >= vetQuantCliente[t].prob)
                        {

                            int cliente = vetQuantCliente[t].cliente;
                            clienteAdd = cliente;
                            EvRoute &evRouteSP = instance.shortestPath->getEvRoute(cliente);
                            auto shortestPath = (instance.shortestPath->getShortestPath(cliente));

                            //cout << "DISTANCIA EV ROUTE SHORTEST PATH: : " << evRouteSP.distancia << "\n\n";

                            if(shortestPath.distIdaVolta < DOUBLE_INF)
                            {

                                // NOVO
                                //vetQuantCliente[t].quant -= 1;

                                addRotaCliente(sol, instance, evRouteSP, cliente);

                                //string str;
                                //evRouteSP.print(str, instance, true);
                                //cout << "ROTA: " << str << "\n";
                                //cout << "ADD rota com cliente " << vetQuantCliente[t].cliente << "\n\n";
                                clientesAdd += 1;
                            }

                            add = true;
                        }

                        if(clientesAdd >= parametros.numMaxClie)
                            break;
                    }

                    t += 1;

                    if(igual && t == next)
                        break;
                    else
                        t = t % instance.getNClients();
                }

            }

            if(add)
            {
                //cout << "\n\n";
            }

            //sol.print(instance);

        }

        somaProb = 0.0;
        posAlfa = 0;

        valAleatorio = rand_u32() % 100;

        for(int j=0;somaProb < valAleatorio; ++j)
        {

            if(j >= tamAlfa)
            {
                break;
            }

            somaProb+= (vetorProbabilidade[j]);
            posAlfa = j;
        }


        alfa = parametros.vetAlfa[posAlfa];
        vetorFrequencia[posAlfa] += 1;
        construtivo(sol, instance, alfa, alfa);

        if(!sol.viavel && parametros.iteracoesCalProb > 0)// && i < parametros.iteracoesCalProb)
        {
            int quantCliInv = 0;
            for(int t=instance.getFirstClientIndex(); t <= instance.getEndClientIndex(); ++t)
            {
                if(sol.vetClientesAtend[t] == 0)
                {

                    quantCliInv += 1;
                    //cout<<"\tCLI: "<<t<<"\n";
                    const EvRoute &evRouteAux = instance.shortestPath->getEvRoute(t);
                    if(evRouteAux.routeSize > 2)
                    {
                        vetQuantCliente[convIndClienteVet(t)].add1Quant();
                    }

                }
            }

        }


        if(sol.viavel)
        {
            //cout<<"VIAVEL\n";
            string erro;

            if(!sol.checkSolution(erro, instance))
            {

                cout<<"\n\nSOLUCAO:\n\n";
                //sol.print(instance);

                cout <<erro<< "\n****************************************************************************************\n\n";
                //delete solBest;
                //throw "ERRO";
            }
            else
            {

                estat.numSol += 1;
                estat.distAcum += sol.distancia;

                if(sol.distancia < solBest->distancia || !solBest->viavel)
                {
                    solBest->copia(sol);
                    custoBest = solBest->distancia;
                    //solBest->print(instance);
                    //cout<<"i: "<<i<<"\n";

                }

                solucaoAcumulada[posAlfa] += sol.distancia;

            }

        }
        else if(!solBest->viavel && !sol.viavel)
        {
            static bool fist = true;


            {
                //cout<<"ATUAL\n";
                solBest->copia(sol);

                double aux = sol.distancia + getPenalidade(sol, instance, fator);
                if(aux < custoBest)
                    custoBest = aux;

/*                cout<<"\n\n***************************************************************************\n";
                sol.print(instance);
                solBest->print(instance);
                cout<<"\n\n***************************************************************************!!\n";*/


                fist = false;

            }
        }

        if(!sol.viavel)
            solucaoAcumulada[posAlfa] += sol.distancia + getPenalidade(sol, instance, fator);

        if(i>0 && (i%parametros.numAtualProbReativo)==0)
            atualizaProb();


    }
    // END FOR GRASP


    if(solBest->viavel)
    {
        estat.erro = "";
        if(!solBest->checkSolution(estat.erro, instance))
        {
            cout<<"\n\nSOLUCAO:\n\n";
            solBest->print(instance);

            cout << estat.erro<< "\n****************************************************************************************\n\n";
            delete solBest;
            return nullptr;
        }
    }
    return solBest;

}

double NameS_Grasp::getPenalidade(Solucao &sol, Instance &instancia, float f)
{
    static double penalidade = 1.2*instancia.penalizacaoDistEv;


    if(sol.viavel)
        return 0.0;

    int num = 0;
    for(int i=instancia.getFirstClientIndex(); i <= instancia.getEndClientIndex(); ++i)
    {
        if(sol.vetClientesAtend[i] == 0)
            num += 1;
    }

    return f*num*penalidade;
}

double NameS_Grasp::getDistMaisPenalidade(Solucao &sol, Instance &instancia)
{
    static double penalidade = 1.2*instancia.penalizacaoDistEv;// + instancia.penalizacaoDistComb;

    if(sol.viavel)
        return sol.distancia;

    int num = 0;
    for(int i=instancia.getFirstClientIndex(); i <= instancia.getEndClientIndex(); ++i)
    {
        if(sol.vetClientesAtend[i] == 0)
            num += 1;
    }

    return sol.distancia + num*penalidade;
}

void NameS_Grasp::inicializaSol(Solucao &sol, Instance &instance)
{

    std::vector<EstDist> vetEstDist(instance.getN_RechargingS());
    EvRoute evRoute(-1, instance.getFirstEvIndex(), instance.getEvRouteSizeMax(), instance);

    for(int i=instance.getFirstSatIndex(); i <= instance.getEndSatIndex(); ++i)
    {
        int vetTam = 0;
        evRoute.satelite = i;

        for(int r=instance.getFirstRechargingSIndex(); r <= instance.getEndRechargingSIndex(); ++r)
        {
            vetEstDist[vetTam].estacao = r;
            vetEstDist[vetTam].distancia = instance.getDistance(i, r);
            ++vetTam;
        }

        std::sort(vetEstDist.begin(), vetEstDist.begin()+vetTam);

        const int numEst = min(NUM_EST_INI, instance.getN_RechargingS());
        const int pos = (rand_u32()%numEst);
        int p = pos;
        evRoute[0].cliente = evRoute[2].cliente = i;

        double taxaConsumoDist = instance.vectVeiculo[instance.getFirstEvIndex()].taxaConsumoDist;

        do
        {
            evRoute[1].cliente = vetEstDist[p].estacao;


            if(vetEstDist[p].distancia * instance.vectVeiculo[instance.getFirstEvIndex()].taxaConsumoDist < instance.vectVeiculo[instance.getFirstEvIndex()].capacidadeBateria)
            {

            }
            p += 1;
        }
        while(p != pos);
    }


}

void NameS_Grasp::addRotaCliente(Solucao &sol, Instance &instancia, const EvRoute &evRoute, const int cliente)
{

    if(evRoute.routeSize <= 2)
        return;

    string str;
    evRoute.print(str, instancia, true);

    int sat = evRoute.satelite;

    int next = 0;

    for(int i=0; i < sol.satelites[sat].tamVetEvRoute; ++i)
    {
        next = i;
        if(sol.satelites[sat].vetEvRoute[i].routeSize <= 2)
            break;
    }

    if(next >= sol.satelites[sat].tamVetEvRoute)
        return;

    if(sol.satelites[sat].vetEvRoute[next].routeSize > 2)
        return;

    int aux = sol.satelites[sat].vetEvRoute[next].idRota;
    sol.satelites[sat].vetEvRoute[next].copia(evRoute);
    sol.satelites[sat].vetEvRoute[next].idRota = aux;


    // Atualiza distacia e demanda
    double dist = sol.satelites[sat].vetEvRoute[next].distancia;
    //cout<<"DIST: "<<dist<<"\n";
    sol.satelites[sat].distancia += dist;
    sol.distancia += dist;
    sol.satelites[sat].demanda   += instancia.vectCliente[cliente].demanda;

    //

    sol.satelites[sat].tamVetEvRoute += 1;
    sol.vetClientesAtend[cliente] = 1;
    sol.solInicializada = true;

}