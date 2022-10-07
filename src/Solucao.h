#ifndef INC_2E_EVRP_SOLUCAO_H
#define INC_2E_EVRP_SOLUCAO_H
#include <vector>
#include "Satelite.h"

class Solucao
{
public:
    Solucao(Instance& Inst);
    Solucao(const Solucao &solution);
    void copia(Solucao &solution);

    int getNSatelites() const;
    int findSatellite(int id) const;
    Satelite* getSatelite(int index);
    bool checkSolution(std::string &erro, Instance &inst);
    void print(std::string &saida, const Instance &instance);
    void print(const Instance& Inst);
    double calcCost(const Instance&);
    float getDistanciaTotal();
    void atualizaVetSatTempoChegMax( Instance &instance);
    void inicializaVetClientesAtend(Instance &instance);
    double getDist1Nivel();
    double getDist2Nivel();
    void recalculaDist();

    // Possui numSat + 1 !!
    std::vector<Satelite> satelites;

    // Guarda o ultimo tempo de chegada do veiculo do 1° nivel
    std::vector<double> satTempoChegMax;
    int numTrucksMax = -1;
    int numEvMax     = -1;
    int numEv        = 0;                   // Num Ev utilizados
    bool viavel      = true;
    std::vector<Route> primeiroNivel;
    bool mvShiftIntraRota = false;
    bool mvShiftInterRotas = false;
    bool mvCross = false;
    double distancia = 0.0;
    std::vector<int8_t> vetClientesAtend;

    bool solInicializada = false;
    int ultimaA = -1;

};
#endif //INC_2E_EVRP_SOLUCAO_H
