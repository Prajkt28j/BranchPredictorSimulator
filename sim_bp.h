#ifndef SIM_BP_H
#define SIM_BP_H


typedef struct bp_params{
	unsigned long int K; //Hybrid Model, K bit to select sither bimodal or gshare
	unsigned long int M1; //Bimodal
	unsigned long int M2; //Gshare
	unsigned long int N;
	char*             bp_name;
}bp_params;


// Put additional data structures here as per your requirement

extern char TAKEN ;
extern char NOT_TAKEN;

enum bp_type {  none=0, bimodal=4, gshare=5, hybrid=7 };
enum counter {  SN = 0, //Strongly Not Taken: 00
				WN = 1, //Weakly Not Taken: 01
				WT = 2, //Weakly Taken: 10
				ST = 3  //Strongly Taken: 11
		     };


namespace Proj_BP{

class BimodalPredictor {

private:
	unsigned long int 		m_size;                //size of prediction counter table
	unsigned short int 		*m_PCT;                //Prediction Counters' Table, part of BTB
	unsigned long int 		m_pc_mask;

	unsigned long int 	  	m_total_req_count;     //total count of BTB requests
	unsigned long int		m_mispred_count;      //count of mispredictions
	double 					m_misPred_rate;       //misprediction rate in %

public:
	//constructor
	BimodalPredictor(unsigned long int);
	void trainAndPredict(char, unsigned long int);
	void displayResult(void);
};

class GSharePredictor {

private:
	unsigned long int     	m_M1;
	unsigned long int     	m_N;
	unsigned long int 		m_size; 			//size of prediction counter table
	unsigned short int 		*m_PCT; 			//Prediction Counters' Table, part of BTB
	unsigned long int 		m_BHR; 				//Branch History Register

	unsigned long int 		m_pc_mask_M;
	unsigned long int 		m_pc_mask_N;

	unsigned long int 	  	m_total_req_count; 	// total count of BTB requests
	unsigned long int		m_mispred_count; 	//count of mispredictions
	double 					m_misPred_rate; 	//misprediction rate in %

public:
	GSharePredictor(unsigned long int, unsigned long int);
	void trainAndPredict(char, unsigned long int);
	void displayResult(void);
};


class HybridPredictor{

private:
	unsigned long int 		m_K, m_M1, m_N, m_M2; //Defines dimensions

	unsigned long int 		m_size_CCT;     	//size of Chooser Counter Table
	unsigned long int 		m_size_BPT;     	//size of Bimodal Prediction Table
	unsigned long int 		m_size_GPT;     	//size of GSHARE Prediction Table

	unsigned short int 		*m_ChooserCT;       //Handle of Chooser Counter Table
	unsigned short int 		*m_BimodalPT;       //Handle of Bimodal Prediction Table
	unsigned short int 		*m_GSharePT;        //Handle of GSHARE Prediction Table

	//  Proj_BP::BimodalPredictor *m_b_bp;      //bimodal branch predictor
	//  Proj_BP::GSharePredictor  *m_g_bp;      //gshare branch predictor

	unsigned long int 		m_mask_K;
	unsigned long int 		m_mask_M1;
	unsigned long int 		m_mask_N;
	unsigned long int 		m_mask_M2;

	unsigned long int 	    m_BHR; 					//Branch History Register

	unsigned long int 	    m_total_req_count; 		// total count of BTB requests
	unsigned long int		m_mispred_count;	 	//count of mispredictions
	double 				    m_misPred_rate; 		//misprediction rate in %

public:
	HybridPredictor(unsigned long int, unsigned long int, unsigned long int, unsigned long int);
	void trainAndPredict(char, unsigned long int);
	void displayResult(void);
};


} //end Proj_BP

#endif
