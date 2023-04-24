import os
import subprocess
import sys
import pandas as pd
import math
import time


#'''
if(len(sys.argv) < 2 and len(sys.argv) > 3):
    print("NUMERO DE ARGUMENTOS ESTA ERRADO.\npython3 teste.py caminho")
    exit(-1)
#'''
   
print("PID: ",os.getpid())
print("\n")

#instanciasTeste = "instanciasTeste.txt"
instanciasTeste = ""

#tamanhoInst = ['15']#, '100_0']
#tamanhoInst = ['5', '10', '15'] #, '100_0']#, '100_1', '100_2']
#tamanhoInst = ['15', '100_0']
#tamanhoInst = ['100_0']
#tamanhoInst = ['100_1']
tamanhoInst = ['100_2']
#tamanhoInst = ['100_1', '100_2']



numExecucoes = 30
caminhoDir = str(sys.argv[1])
#caminhoDir = ''

parametros = " --pasta '" + caminhoDir+ "' --resulCSV 'resultados.csv' --execTotal "+str(numExecucoes)+ " --alphaSeg 0.05 --betaPrim 0.8 --difBest 0.01 --numItIG 3000 --torneio 0 --taxaRm 0.2" + " --execAtual "

print("PARAMETROS: \n", parametros, "\n")

    


diretorioIni = 'instancias/2e-vrp-tw/'
instanciasVet = []
numExecucoesVet = []

print(instanciasTeste)

if not instanciasTeste:

	strInstancias = ""
	for i in tamanhoInst:
    		strInstancias += i + " "

	#print("INSTANCIAS: ", strInstancias, "\n")
	files = []
	for i in tamanhoInst:
		#print('Customer_' + str(i))
		caminho = diretorioIni + 'Customer_' + str(i) + '/'
		print(caminho)
		files = os.listdir(caminho)
		files.sort()

		for f in files:
			instanciasVet.append(caminho+f)
			numExecucoesVet.append(0)

else:
	inst = pd.read_csv(instanciasTeste)
	n = len(inst.index)
	strInst = "\t"
	for i in range(n):
		strTemp = inst.loc[i,'inst']
		if strTemp[0] == '#':
			continue
		
		strInst = strInst + inst.loc[i,'inst'] + "\n\t"
		caminho = diretorioIni + 'all/' + inst.loc[i,'inst'] + '.txt'
		instanciasVet.append(caminho)
		numExecucoesVet.append(0)
		
	print('Instancias:\n', strInst)


instancias = pd.DataFrame()
instanciasCsv = caminhoDir+"/instancias.csv"

if(len(sys.argv) == 3):
    instancias = pd.read_csv(instanciasCsv)
    print(instancias)
    print("\n\n")
    #instancias = instancias.drop(columns=['0'])
    
else:
    temp = {'instancia' : instanciasVet, 'prox' : numExecucoesVet}
    instancias = pd.DataFrame(data=temp)
    instancias.to_csv(instanciasCsv, index=False)
    

sys.stdout.flush()


        
        
n = len(instancias.index)
for j in range(n):
    
    instancia = instancias.loc[j,'instancia']
    start = int(instancias.loc[j,'prox'])
    
    for i in range(start, numExecucoes, 1):

      strExecutavel = caminhoDir+'//run ' + str(instancia) + parametros + str(i)
      os.system(strExecutavel)
      #print(strExecutavel)
      instancias.loc[j,'prox'] = i+1
      instancias.to_csv(instanciasCsv, index=False)
      
    #print(instancias)
    print("\n\n")
    sys.stdout.flush()
