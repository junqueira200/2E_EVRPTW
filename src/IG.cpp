//
// Created by igor on 09/01/23.
//

/* ****************************************
 * ****************************************
 *  Nome:    Igor de Andrade Junqueira
 *  Data:    09/01/23
 *  Arquivo: IG.cpp
 * ****************************************
 * ****************************************/

#include "IG.h"
#include "Construtivo.h"
#include "Construtivo2.h"
#include "Vnd.h"
#include "mersenne-twister.h"
#include <unordered_set>
#include "VetorHash.h"
#include <sys/stat.h>
#include <fstream>

using namespace NameS_IG;
using namespace NameS_Grasp;
using namespace NS_Construtivo;
using namespace NS_Construtivo2;
using namespace NS_vnd;
using namespace NS_VetorHash;

#define PRINT_IG TRUE

Solucao* NameS_IG::iteratedGreedy(Instancia &instancia, ParametrosGrasp &parametrosGrasp, NameS_Grasp::Estatisticas &estat,
                                  const ublas::matrix<int> &matClienteSat, BoostC::vector<NS_vnd::MvValor> &vetMvValor,
                                  BoostC::vector<NS_vnd::MvValor> &vetMvValor1Nivel, NS_parametros::ParametrosSaida &parametrosSaida,
                                  NS_parametros::Parametros &parametros)
{
// NS_parametros::Parametros &parametros
    std::unordered_set<VetorHash, VetorHash::HashFunc> hashSolSetCorrente;    // Solucao Corrente
    int numSolCorrente = 0;

    std::unordered_set<VetorHash, VetorHash::HashFunc> hashSolSetConst;       // Apos construtivo
    std::unordered_set<VetorHash, VetorHash::HashFunc> hashSolSetVnd;         // Apos vnd
    int numSolConstVia = 0;

    // Gera uma sol inicial com grasp
    NS_parametros::ParametrosSaida parametrosSaidaGrasp = parametrosSaida;
    //ParametrosGrasp parametrosGrasp = parametros;
    //parametrosGrasp.numIteGrasp = 300;

    Solucao *solG = grasp(instancia, parametrosGrasp, estat, true, matClienteSat, vetMvValor, vetMvValor1Nivel, parametrosSaidaGrasp);

    if(!solG->viavel)
        return nullptr;

    rvnd(*solG, instancia, 0.4, vetMvValor, vetMvValor1Nivel);

    Solucao solBest(instancia);
    solBest.copia(*solG);
    delete solG;
    solG = nullptr;

#if PRINT_IG
cout<<"GRASP: "<<solBest.distancia<<"\n\n";
#endif

    Solucao solC(instancia);
    solC.copia(solBest);
    int temp = 0;

    for(int sat=1; sat <= instancia.numSats; ++sat)
    {
        for(int ev=0; ev < instancia.numEv; ++ev)
        {
            EvRoute &evRoute = solBest.satelites[sat].vetEvRoute[ev];
            if(evRoute.routeSize > 2)
                temp += 1;
        }
    }

    const int numEvN_Vazias = temp;

    //Parametros
    const float alfa  = 0.8; //0.8
    const float beta  = 0.8;

    const int numEvRm = min(int(0.1*numEvN_Vazias+1), 5);
    const int numItSemMelhoraResetSolC = 20;
    int ultimaA = 0;
    int numSolG = 1;
    int numFuncDestroi = 0;
    const int numChamadasDestroi0 = int(NS_Auxiliary::upperVal(numEvN_Vazias/float(numEvRm)));
    //const int numChamadasDestroi0 = 1;

#if PRINT_IG
    cout<<"destroi0 num chamadas: "<<numChamadasDestroi0<<"\n";
#endif

    //cout<<"NUM EV: "<<numEvRm<<"\n\n";

    // Escolhe aleatoriamente numRotas nao vazias e as removem da solucao
    auto funcDestroi0 = [&](Solucao &sol, const int numRotas)
    {
        for(int i=0; i < numRotas; ++i)
        {
            bool escolheRota = false;

            // Percorre os sat
            const int satIdPrim = rand_u32() % instancia.numSats;
            int satId = satIdPrim;

            do
            {
                Satelite &sat = sol.satelites[satId];

                // Percorre os EVs
                const int evPrim = rand_u32() % instancia.numEv;
                int ev = evPrim;

                do
                {
                    EvRoute &evRoute = sat.vetEvRoute[ev];
                    if(evRoute.routeSize <= 2)
                    {
                        ev = (ev+1) % instancia.numEv;
                        continue;
                    }

                    //cout<<"\t\t";

                    escolheRota = true;
                    for(int j=1; j < (evRoute.routeSize-1); ++j)
                    {
                        if(instancia.isClient(evRoute[j].cliente))
                        {
                            sol.vetClientesAtend[evRoute[j].cliente] = int8_t(0);
                            //cout<<evRoute[j].cliente<<" ";
                        }
                    }

                    //cout<<"\n\n";

                    // Remove distancia do ev do sat e da sol
                    sat.distancia -= evRoute.distancia;
                    sol.distancia -= evRoute.distancia;

                    // Remove carga do ev do sat e da sol
                    sat.demanda -= evRoute.demanda;
                    int temp = evRoute.idRota;

                    //string strRota;
                    //evRoute.print(strRota, instancia, true);
                    //cout<<"ROTA ANTES: "<<strRota<<"\n";

                    sat.vetEvRoute[ev] = EvRoute(satId, evRoute.idRota, evRoute.routeSizeMax, instancia);
                    sat.vetEvRoute[ev].idRota = temp;

                    //strRota = "";
                    //evRoute.print(strRota, instancia, true);
                    //cout<<"ROTA DEPOIS: "<<strRota<<"\n\n";

                    sol.numEv -= 1;
                    sol.rotaEvAtualizada(satId, ev);
                    break;

                }while(ev != evPrim);

                if(escolheRota)
                    break;

                satId = (satId+1) % instancia.numSats;
            }while(satId != satIdPrim);
        }

        sol.reseta1Nivel(instancia);
        sol.resetaIndiceEv(instancia);

    };

    // Remove aleatoriamente um sat, o sat pode estar vazio(nao faz nada)
    auto funcDestroi1 = [&](Solucao &sol) -> bool
    {
        if(instancia.numSats == 1)
            return false;

        int sat = 1 + (rand_u32()%instancia.numSats);
        Satelite &satelite = sol.satelites[sat];
        if(satelite.vazio())
            return false;

        const int numEvSat = satelite.numEv();
        sol.numEv -= numEvSat;

        double dist = satelite.excluiSatelite(instancia, sol.vetClientesAtend);
        sol.distancia -= dist;
        sol.vetMatSatEvMv[sat] =  ublas::matrix<int>(instancia.numEv, NUM_MV, 0);

        sol.reseta1Nivel(instancia);
        sol.resetaIndiceEv(instancia);

        return true;
    };

    BoostC::vector<DadosIg> vetDadosIg;
    vetDadosIg.reserve(parametrosGrasp.numIteGrasp);

    for(int i=0; i < parametrosGrasp.numIteGrasp; ++i)
    {

#if PRINT_IG
if(i%200 == 0)
    cout<<"ITERACAO: "<<i<<"\n";
#endif

        /*
        if((i-ultimaA) == numItSemMelhoraResetSolC)
        {
            solG = nullptr;
            while(solG == nullptr)
            {
                solG = grasp(instancia, parametrosGrasp, estat, true, matClienteSat, vetMvValor, vetMvValor1Nivel, parametrosSaidaGrasp);
            }

            if(!solG->viavel)
            {
                cout<<"SOL INVIAVEL!\n";
                throw "ERRO";
            }

            rvnd(*solG, instancia, beta, vetMvValor, vetMvValor1Nivel);


            solC = Solucao(instancia);
            solC.copia(*solG);
            ultimaA = i;
            numFuncDestroi = 0;
            delete solG;
        }
        else if
        */

        if((i-ultimaA) % numItSemMelhoraResetSolC == 0 && i!=ultimaA)
        {
            solC = Solucao(instancia);
            solC.copia(solBest);
        }

        hashSolSetCorrente.insert(VetorHash(solC, instancia));
        numSolCorrente += 1;
        DadosIg dadosIg;

        dadosIg.it = i;
        dadosIg.solCorrente = solC.distancia;

//cout<<"SOL "<<i<<": "<<solC.distancia<<"\n";

        if(numFuncDestroi < numChamadasDestroi0)
        {
            funcDestroi0(solC, numEvRm);
            numFuncDestroi += 1;
        }
        else if(numFuncDestroi == numChamadasDestroi0)
        {
            if(!funcDestroi1(solC))
                funcDestroi0(solC, numEvRm);

            numFuncDestroi = 0;
        }

//cout<<"***********************************\n\n";


        construtivo(solC, instancia, alfa, beta, matClienteSat, true, true, false);
        //construtivo2(solC, instancia, alfa, beta, matClienteSat, true, true);

        if(!solC.viavel)
        {

            dadosIg.solConst = -1.0;
            dadosIg.solVnd   = -1.0;

            solC = Solucao(instancia);
            solC.copia(solBest);
//cout<<"\tSOL INVIAVEL\n";

        }
        else
        {
            string strErro;
            if(!solC.checkSolution(strErro, instancia))
            {
                PRINT_DEBUG("", "");
                cout<<"ERRO NO IG APOS O CONSTRUTIVO\nERRO:\n\n";
                cout<<strErro<<"\n\n";
                throw "ERRO";
            }

            hashSolSetConst.insert(VetorHash(solC, instancia));
            numSolConstVia += 1;

            dadosIg.solConst = solC.distancia;

            numSolG += 1;
            rvnd(solC, instancia, beta, vetMvValor, vetMvValor1Nivel);

            hashSolSetVnd.insert(VetorHash(solC, instancia));
            dadosIg.solVnd   = solC.distancia;

//cout<<"\tSOL VIAVEL "<<i<<": "<<solC.distancia<<"\n";
        }


        if(NS_Auxiliary::menor(solC.distancia, solBest.distancia))
        {
            string erro;
            if(!solC.checkSolution(erro, instancia))
            {
                PRINT_DEBUG("", "");
                cout<<"ERRO, IG\n";
                cout<<erro<<"\n\n";
                throw "ERRO";
            }

            solBest = Solucao(instancia);
            solBest.copia(solC);
            solBest.ultimaA = i;
            numFuncDestroi = 0;
            ultimaA = i;

#if PRINT_IG
cout<<"ATUALIZACAO "<<i<<": "<<solBest.distancia<<"\n\n";
#endif

        }

        dadosIg.solBest = solBest.distancia;
        vetDadosIg.push_back(dadosIg);

    } // END for ig

    double fatorSolCorr = (double(hashSolSetCorrente.size())/numSolCorrente) * 100;
    double fatorSolConst = (double(hashSolSetConst.size())/numSolConstVia) * 100;
    double fatorSolVnd = (double(hashSolSetVnd.size())/numSolConstVia) * 100;

//cout<<"Sol Corrente: "<<fatorSolCorr<<"\n";
//cout<<"Sol Construtivo: "<<fatorSolConst<<"\n";
//cout<<"Sol VND: "<<fatorSolVnd<<"\n\n";

    auto funcAddParaSaida = [&](string &&strParm, double val)
    {

        parametrosSaida.mapNoSaida[strParm] = NS_parametros::NoSaida(strParm);
        parametrosSaida.mapNoSaida[strParm].addSaida(SAIDA_EXEC_VAL);
        parametrosSaida.mapNoSaida[strParm](val);
    };

    funcAddParaSaida("numSol", numSolG);
    funcAddParaSaida("fatorSolCorr", fatorSolCorr);
    funcAddParaSaida("fatorSolConst", fatorSolConst);
    funcAddParaSaida("fatorSolVnd", fatorSolVnd);

    string erro;
    if(!solBest.checkSolution(erro, instancia))
    {

        PRINT_DEBUG("", "");
        cout<<"ERRO, IG\n";
        cout<<erro<<"\n\n";
        throw "ERRO";
    }

#if PRINT_IG
cout<<"IG: "<<solBest.distancia<<"\n\n";
#endif

    printVetDadosIg(vetDadosIg, parametros);

    Solucao *solPtr = new Solucao(instancia);
    solPtr->copia(solBest);
    return solPtr;
}

void NameS_IG::printVetDadosIg(BoostC::vector<DadosIg> &vetDadosIg, NS_parametros::Parametros &parametros)
{
    if(vetDadosIg[0].solConst < 0.0)
    {
        vetDadosIg[0].solConst = vetDadosIg[0].solCorrente;
        vetDadosIg[0].solVnd   = vetDadosIg[0].solCorrente;
    }

    for(int i=1; i < vetDadosIg.size(); ++i)
    {
        if(vetDadosIg[i].solConst < 0.0)
            vetDadosIg[i].solConst = vetDadosIg[i-1].solConst;

        if(vetDadosIg[i].solVnd < 0.0)
            vetDadosIg[i].solVnd = vetDadosIg[i-1].solVnd;
    }

    string strVet = "it, corr, const, vnd, best\n";
    const int ultimaIt = parametros.numItTotal-1;

    for(auto it:vetDadosIg)
    {
        strVet += to_string(it.it) + ", " + to_string(it.solCorrente) + ", " + to_string(it.solConst) + ", " + to_string(it.solVnd) +
                + ", " + to_string(it.solBest) + "\n";
    }


    if(mkdir(parametros.caminhoPasta.c_str(), 0777) == -1)
    {
        if(errno != EEXIST)
        {
            cout << "ERRO, NAO FOI POSSIVEL CRIAR O DIRETORIO: " << parametros.caminhoPasta << "\nErro: " << strerror(errno) << "\n\n";
            throw "ERRO";
        }
    }

    const string caminhoInst = parametros.caminhoPasta + "/dadosIg/";

    if(mkdir(caminhoInst.c_str(), 0777) == -1)
    {
        if(errno != EEXIST)
        {
            cout << "ERRO, NAO FOI POSSIVEL CRIAR O DIRETORIO: " << caminhoInst << "\nErro: " << strerror(errno) << "\n\n";
            throw "ERRO";
        }
    }


    const string caminho = caminhoInst + parametros.nomeInstancia + "/";

    if(mkdir(caminho.c_str(), 0777) == -1)
    {
        if(errno != EEXIST)
        {
            cout << "ERRO, NAO FOI POSSIVEL CRIAR O DIRETORIO: " << caminho << "\nErro: " << strerror(errno) << "\n\n";
            throw "ERRO";
        }
    }
    //cout<<"DIRETORIO: "<<caminho<<" criado\n";

    string strSaida = caminho+parametros.nomeInstancia+"_EXE_"+ to_string(parametros.execucaoAtual)+".csv";

    std::ofstream outfile;
    outfile.open(strSaida, std::ios_base::out);

    if(outfile.is_open())
    {
        outfile<<strVet<<"\n";
        outfile.close();
    }
    else
    {
        cout<<"NAO FOI POSSIVEL ABRIR O ARQUIVO: "<<strSaida<<"\n";
    }

}
