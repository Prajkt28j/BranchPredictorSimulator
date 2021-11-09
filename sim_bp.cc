#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_bp.h"

#include <math.h>

using namespace std;
using namespace Proj_BP;


 char TAKEN 		= 't';
 char NOT_TAKEN 	= 'n';


/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim bimodal 6 gcc_trace.txt
    argc = 4
    argv[0] = "sim"
    argv[1] = "bimodal"
    argv[2] = "6"
    ... and so on
*/
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file

    bp_type predictor = none;
            params.M1 = 0;
            params.M2 = 0;
            params.N  = 0;
            params.K  = 0;

    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }

    params.bp_name  = argv[1];

    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if(strcmp(params.bp_name, "bimodal") == 0)              // Bimodal
    {
        if(argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M2       = strtoul(argv[2], NULL, 10);
        trace_file      = argv[3];
        predictor       = bimodal;
        printf("COMMAND\n %s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
    }
    else if(strcmp(params.bp_name, "gshare") == 0)          // Gshare
    {
        if(argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M1       = strtoul(argv[2], NULL, 10);
        params.N        = strtoul(argv[3], NULL, 10);
        trace_file      = argv[4];
        predictor       = gshare;
        printf("COMMAND\n %s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);
    }
    else if(strcmp(params.bp_name, "hybrid") == 0)          // Hybrid
    {
        if(argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.K        = strtoul(argv[2], NULL, 10);
        params.M1       = strtoul(argv[3], NULL, 10);
        params.N        = strtoul(argv[4], NULL, 10);
        params.M2       = strtoul(argv[5], NULL, 10);
        trace_file      = argv[6];
        predictor       = hybrid;
        printf("COMMAND\n %s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);

    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    //sim bimodal <M2> <tracefile>
    Proj_BP::BimodalPredictor bimodal_bp(params.M2);

    //sim gshare <M1> <N> <tracefile>
    Proj_BP::GSharePredictor  gshare_bp(params.M1, params.N);

    //sim hybrid <K> <M1> <N> <M2> <tracefile>
    Proj_BP::HybridPredictor  hybrid_bp(params.K, params.M1, params.N, params.M2);

    char str[2];
    while(fscanf(FP, "%lx %s", &addr, str) != EOF)
    {

        outcome = str[0];
      /*  if (outcome == 't')
            printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
        else if (outcome == 'n')
            printf("%lx %s\n", addr, "n");  */        // Print and test if file is read correctly
        /*************************************
            Add branch predictor code here
        **************************************/

        switch(predictor)
        {
           case ::bimodal: //4
                bimodal_bp.trainAndPredict(outcome, addr);
                break;

           case ::gshare: //5
                gshare_bp.trainAndPredict(outcome, addr);
                break;

           case ::hybrid: //7
                hybrid_bp.trainAndPredict(outcome, addr);
                break;
          default:
                printf("Error: Wrong number of arguments from command line:%d\n", predictor);
                exit(EXIT_FAILURE);
        }
    }

    //To display results according to validation runs
    switch(predictor)
    {
       case ::bimodal:
            bimodal_bp.displayResult();
            break;

       case ::gshare:
            gshare_bp.displayResult();
            break;

       case ::hybrid:
            hybrid_bp.displayResult();
            break;
      default:
            printf("Error: Wrong number of arguments from command line:%d\n", predictor);
            exit(EXIT_FAILURE);
    }
    return 0;
}



namespace Proj_BP{

/******************************** BIMODAL Branch Predictor **********************************/


//constructor
BimodalPredictor::BimodalPredictor(unsigned long int t_m_bits) {
	//printf("BimodalPredictor::BimodalPredictor");

	m_size = pow(2, t_m_bits);
	m_PCT  = new unsigned short int[m_size];
	for (unsigned long int b = 0; b < m_size; b++)  m_PCT[b] = 2;
	m_pc_mask = (1 << t_m_bits) - 1; //bimodal uses only mbits of pc for indexing in BHT

	m_total_req_count 	= 0;
	m_mispred_count 	= 0;
	m_misPred_rate 		= 0;

}

void BimodalPredictor::trainAndPredict( char t_actual_br_outcome, unsigned long int t_pc ){
	//	printf("BimodalPredictor::trainAndPredict");

	unsigned long int id;
	bool predicted_br_outcome = false; //TRUE: Taken, FALSE: Not Taken

	++m_total_req_count;
	id = (t_pc >> 2) & m_pc_mask; //2 LSB Bits of PC are always 0 hence discard


	if( (::SN == m_PCT[id]) || (::WN == m_PCT[id]) ) //0,1
	{
		predicted_br_outcome = false; //Predict Not taken
	}
	else if( (::WT == m_PCT[id]) || (::ST == m_PCT[id]) ) //2,3
	{
		predicted_br_outcome = true; // If previous value of counter is 2/3 predict current as well Taken
	}

	if ( TAKEN == t_actual_br_outcome )
	{
		if (::ST != m_PCT[id]) ++m_PCT[id]; //increment counter on actual outcome is taken
		if (false == predicted_br_outcome) ++m_mispred_count;
	}
	else if ( NOT_TAKEN == t_actual_br_outcome)
	{
		if (::SN != m_PCT[id]) --m_PCT[id]; //decrement counter on actual outcome is non-taken
		if (true == predicted_br_outcome) ++m_mispred_count;
	}
}


void BimodalPredictor::displayResult(void) {
	//	printf("BimodalPredictor::displayResult");

	m_misPred_rate = ( (double(m_mispred_count) / m_total_req_count) * 100 );

	printf("OUTPUT\n");
	printf(" number of predictions:    %lu\n", 				m_total_req_count);
	printf(" number of mispredictions: %lu\n", 				m_mispred_count);
	printf(" misprediction rate:       %0.2lf%%\n",	  		m_misPred_rate);
	printf("FINAL BIMODAL CONTENTS\n");
	for (unsigned long int b = 0; b < m_size; b++) printf(" %lu\t%u\n", b, m_PCT[b]);
}


/******************************** G-SHARE Branch Predictor **********************************/

//constructor
GSharePredictor::GSharePredictor(unsigned long int t_m, unsigned long int t_n)
{
	m_M1 				= t_m;
	m_N 				= t_n;
	m_pc_mask_M 		= (1 << (m_M1 - m_N)) - 1;
	m_pc_mask_N 		= (1 << m_N) - 1;

	m_size	  			= pow(2, m_M1);
	m_PCT 				= new unsigned short int[m_size];
	for (unsigned long int b = 0; b < m_size; b++)  m_PCT[b] = ::WT; //2;

	m_total_req_count   = 0;
	m_mispred_count 	= 0;
	m_misPred_rate 		= 0;
	m_BHR				= 0;
}


void GSharePredictor::trainAndPredict( char t_actual_br_outcome, unsigned long int t_pc){

	++m_total_req_count;

	unsigned long int id;
	unsigned long int pattern, masked_addr, x, y;

	bool predicted_br_outcome = false; //TRUE: Taken, FALSE: Not Taken

	pattern 	= (m_BHR & m_pc_mask_N);
	masked_addr = (t_pc >> (2 + m_M1 - m_N) & m_pc_mask_N);
	x 			= pattern ^ masked_addr;
	y 			= m_pc_mask_M & (t_pc >> 2);
	id 			= (y | (x << (m_M1 - m_N)));


	if( (m_PCT[id] == ::SN) || (m_PCT[id] == WN) ) // 0,1
	{
		predicted_br_outcome = false; //Predict Not taken
	}
	else if( (m_PCT[id] == ::WT) || (m_PCT[id] == ::ST) ) //2,3
	{
		predicted_br_outcome = true; // If previous value of counter is 2/3 predict current as well Taken
	}


	if ( TAKEN == t_actual_br_outcome )
	{
		if (m_PCT[id] != ::ST) m_PCT[id]++; //increment counter on taken actual outcome
		if (false == predicted_br_outcome) ++m_mispred_count;

		m_BHR = (m_BHR >> 1) | (1 << (m_N - 1));
	}
	else if ( NOT_TAKEN == t_actual_br_outcome )
	{
		if (m_PCT[id] != ::SN) --m_PCT[id];  //decrement counter on Non-taken actual outcome
		if (true == predicted_br_outcome) ++m_mispred_count;

		m_BHR = (m_BHR >> 1) & ((1 << m_N) - 1);
	}

}

void GSharePredictor::displayResult(void) {
	//	printf("GSharePredictor::displayResult");

	m_misPred_rate = ( (double(m_mispred_count) / m_total_req_count) * 100 );

	printf("OUTPUT\n");
	printf(" number of predictions:    %lu\n", 				m_total_req_count);
	printf(" number of mispredictions: %lu\n", 				m_mispred_count);
	printf(" misprediction rate:       %0.2lf%%\n",	  m_misPred_rate);
	printf("FINAL GSHARE CONTENTS\n");
	for (unsigned long int b = 0; b < m_size; b++) printf(" %lu\t%u\n", b, m_PCT[b]);
}


/******************************** Hybrid Branch Predictor **********************************/


//constructor
HybridPredictor::HybridPredictor(unsigned long int t_K,
		unsigned long int t_M1,
		unsigned long int t_N,
		unsigned long int t_M2) {
	m_K         = t_K;
	m_M2        = t_M2;
	m_M1        = t_M1;
	m_N         = t_N;

	m_mask_K    = (1 << m_K) - 1;
	m_mask_M2   = (1 << m_M2) - 1;
	m_mask_M1   = (1 << (m_M1 - m_N)) - 1;
	m_mask_N    = (1 << m_N) - 1;

	m_size_CCT  = pow(2, m_K);
	m_ChooserCT = new unsigned short int[m_size_CCT];
	for (unsigned long int b = 0; b < m_size_CCT; b++)  m_ChooserCT[b] = ::WN; //1;

	m_size_BPT  = pow(2, m_M2);
	m_BimodalPT = new unsigned short int[m_size_BPT];
	for (unsigned long int b = 0; b < m_size_BPT; b++)  m_BimodalPT[b] = ::WT; //2;

	m_size_GPT = pow(2, m_M1);
	m_GSharePT = new unsigned short int[m_size_GPT];
	for (unsigned long int b = 0; b < m_size_GPT; b++)  m_GSharePT[b] = ::WT; //2;

	m_total_req_count 	= 0;
	m_mispred_count 	= 0;
	m_misPred_rate 		= 0;
	m_BHR 				= 0;

}


void HybridPredictor::trainAndPredict( char t_actual_br, unsigned long int t_pc ){

	++m_total_req_count;

	unsigned long int id_Hybrid, id_Bimodal, id_GShare;
	unsigned long int pattern, masked_addr, x, y;

	bool choose 				= false; 					// TRUE: GSHARE, FALSE: BIMODAL
	bool bimodal_pred_outcome 	= false; 		// TRUE: Taken,  FALSE: Not Taken
	bool gshare_pred_outcome 	= false; 		// TRUE: Taken,  FALSE: Not Taken

	//Index of Hybrid Predictor Table
	id_Hybrid 	= m_mask_K & (t_pc >> 2);

	//Index of Bimodal Predictor
	id_Bimodal 	= m_mask_M2 & (t_pc >> 2);

	//Index of GShare Predictor
	pattern 	= (m_BHR & m_mask_N);
	masked_addr = (m_mask_N & (t_pc >> (2 + m_M1 - m_N)));
	x 			= pattern ^ masked_addr;
	y 			= (m_mask_M1 & (t_pc >> 2));
	id_GShare 	= (y | (x << (m_M1 - m_N)));


	if(     (m_GSharePT[id_GShare] == ::ST) || (m_GSharePT[id_GShare] == ::WT)){gshare_pred_outcome = true;}
	else if((m_GSharePT[id_GShare] == ::SN) || (m_GSharePT[id_GShare] == ::WN)){gshare_pred_outcome = false;}

	if(     (m_BimodalPT[id_Bimodal] == ::ST) || (m_BimodalPT[id_Bimodal] == ::WT)){bimodal_pred_outcome = true;}
	else if((m_BimodalPT[id_Bimodal] == ::SN) || (m_BimodalPT[id_Bimodal] == ::WN)){bimodal_pred_outcome = false;}

	if(     (m_ChooserCT[id_Hybrid] == ::ST) || (m_ChooserCT[id_Hybrid] == ::WT)){choose = true;} //gshare
	else if((m_ChooserCT[id_Hybrid] == ::SN) || (m_ChooserCT[id_Hybrid] == ::WN)){choose = false;} //bimodal


	//update predictor and register table
	if ( TAKEN == t_actual_br )
	{
		//update branch history register
		m_BHR = (m_BHR >> 1) | (1 << (m_N - 1)) ;

		if (!choose) //bimodal selected
		{
			if (::ST != m_BimodalPT[id_Bimodal]){ ++m_BimodalPT[id_Bimodal];}
			if (false == bimodal_pred_outcome)  { ++m_mispred_count;}
		}
		else  //gshare selected
		{
			if (::ST != m_GSharePT[id_GShare]){ ++m_GSharePT[id_GShare];}
			if (false == gshare_pred_outcome) { ++m_mispred_count;}
		}


		//if gshare gave correct prediction, increment hybrid counter
		if ((!bimodal_pred_outcome) && gshare_pred_outcome)
		{
			if (::ST != m_ChooserCT[id_Hybrid]) { ++m_ChooserCT[id_Hybrid];}
		}
		else if ((!gshare_pred_outcome) && bimodal_pred_outcome) //if bimodal gave correct prediction, decrement hybrid counter
		{
			if (::SN != m_ChooserCT[id_Hybrid]) { --m_ChooserCT[id_Hybrid];}
		}
	}
	else if( NOT_TAKEN == t_actual_br ){

		//update branch history register
		m_BHR = (m_BHR >> 1) & ((1 << m_N) - 1);

		if (!choose) //If bimodal was selected
		{
			if (::SN != m_BimodalPT[id_Bimodal]){ --m_BimodalPT[id_Bimodal];}
			if (true == bimodal_pred_outcome)   { ++m_mispred_count;}
		}
		else if (choose) //If gshare was selected
		{
			if (::SN != m_GSharePT[id_GShare]){ --m_GSharePT[id_GShare];}
			if (true == gshare_pred_outcome)  { ++m_mispred_count;	}
		}

		//If bimodal gave correct prediction, decrement hybrid counter
		if ((!bimodal_pred_outcome) && gshare_pred_outcome)
		{
			if (::SN != m_ChooserCT[id_Hybrid]) { --m_ChooserCT[id_Hybrid];}
		}
		else if ((!gshare_pred_outcome) && bimodal_pred_outcome)//If gshare gave correct prediction, increment hybrid counter
		{
			if (::ST != m_ChooserCT[id_Hybrid]) { ++m_ChooserCT[id_Hybrid];}
		}
	}

}

void HybridPredictor::displayResult(void) {

	//printf("HybridPredictor::displayResult");

	m_misPred_rate = ( (double(m_mispred_count) / m_total_req_count) * 100 );

	printf("OUTPUT\n");
	printf(" number of predictions:    %lu\n", 				m_total_req_count);
	printf(" number of mispredictions: %lu\n", 				m_mispred_count);
	printf(" misprediction rate:       %0.2lf%%\n",	  		m_misPred_rate);
	printf("FINAL CHOOSER CONTENTS\n");
	for (unsigned long int b = 0; b < m_size_CCT; b++) printf(" %lu\t%u\n", b, m_ChooserCT[b]);
	printf("FINAL GSHARE CONTENTS\n");
	for (unsigned long int b = 0; b < m_size_GPT; b++) printf(" %lu\t%u\n", b, m_GSharePT[b]);
	printf("FINAL BIMODAL CONTENTS\n");
	for (unsigned long int b = 0; b < m_size_BPT; b++) printf(" %lu\t%u\n", b, m_BimodalPT[b]);
}



}//Proj_BP
