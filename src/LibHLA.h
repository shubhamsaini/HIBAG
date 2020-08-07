// ===============================================================
//
// HIBAG R package (HLA Genotype Imputation with Attribute Bagging)
// Copyright (C) 2011-2020   Xiuwen Zheng (zhengx@u.washington.edu)
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// ===============================================================
// Name           : LibHLA
// Author         : Xiuwen Zheng
// Kernel Version : 1.4
// Copyright      : Xiuwen Zheng (GPL v3)
// Description    : HLA imputation C++ library
// ===============================================================

#ifndef LIBHLA_H_
#define LIBHLA_H_


// Detect whether x86 microprocessor architecture or not
#if defined(__i386__) || defined(__X86__) || defined(_M_IX86) || defined(__I86__) || defined(__INTEL__) || defined(__amd64__) || defined(__x86_64__) || defined(_M_AMD64)
#   define HIBAG_CPU_ARCH_X86
#endif

// 32-bit or 64-bit registers
#ifdef __LP64__
#   define HIBAG_CPU_LP64
#else
#   ifdef HIBAG_CPU_LP64
#      undef HIBAG_CPU_LP64
#   endif
#endif

// Function multi-versioning (requiring target_clones)
#if defined(__GNUC__) && (__GNUC__ >= 6) && defined(HIBAG_CPU_ARCH_X86)
#   define HIBAG_HAVE_TARGET_CLONES
#   define HIBAG_TARGET_CLONES    \
        __attribute__((target_clones("default","sse2","sse3","popcnt","avx","avx2")))
#else
#   define HIBAG_TARGET_CLONES
#endif

// Always inline function
#ifndef ALWAYS_INLINE
#   ifdef _MSC_VER
#       define ALWAYS_INLINE    __forceinline
#   elif defined(__GNUC__)
#       define ALWAYS_INLINE    inline __attribute__((always_inline))
#   else
#       define ALWAYS_INLINE    inline
#   endif
#endif


#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <cmath>
#include <vector>
#include <list>
#include <string>
#include <algorithm>


namespace HLA_LIB
{
	using namespace std;

	/// Kernel Version, Major Number (0x01) / Minor Number (0x04)
	#define HIBAG_KERNEL_VERSION    0x0104

	/// Define 8-bit unsigned integer
	typedef uint8_t     UINT8;

	/// The max number of SNP markers in an individual classifier.
	//  Don't modify this value since the code is optimized for this value!!!
	const size_t HIBAG_MAXNUM_SNP_IN_CLASSIFIER = 128;

	/// The max number of UTYPE for packed SNP genotypes.
	const size_t HIBAG_PACKED_UTYPE_MAXNUM =
		HIBAG_MAXNUM_SNP_IN_CLASSIFIER / (8*sizeof(UINT8));


	// ===================================================================== //

	/// macro for checking error
	#define HIBAG_CHECKING(x, msg)	{ if (x) throw ErrHLA(msg); }



	// ===================================================================== //
	// ========                     Description                     ========
	//
	// Packed SNP storage strategy is used for faster matching
	//
	// HLA allele: start from 0
	//
	// THaplotype: packed SNP alleles (little endianness):
	//     (s8 s7 s6 s5 s4 s3 s2 s1)
	//     the 1st allele: (s1), the 2nd allele: (s2), ...
	//     SNP allele: 0 (B allele), 1 (A allele)
	//
	// TGenotype: packed SNP genotype (little endianness):
	//     array_1 = (s1_8 s1_7 s1_6 s1_5 s1_4 s1_3 s1_2 s1_1),
	//     array_2 = (s2_8 s2_7 s2_6 s2_5 s2_4 s2_3 s2_2 s2_1),
	//     array_3 = (s3_8 s3_7 s3_6 s3_5 s3_4 s3_3 s3_2 s3_1)
	//     the 1st genotype: (s1_1 s2_1 s3_1),
	//     the 2nd genotype: (s1_1 s2_1 s3_1), ...
	//     SNP genotype: 0 (BB) -- (s1_1=0 s2_1=0 s3_1=1),
	//                   1 (AB) -- (s1_1=1 s2_1=0 s3_1=1),
	//                   2 (AA) -- (s1_1=1 s2_1=1 s3_1=1),
	//                   -1 or other value (missing)
	//                          -- (s1_1=0 s2_1=0 s3_1=0)
	//
	// ========                                                     ========
	// ===================================================================== //

	/// Packed bi-allelic SNP haplotype structure: 8 alleles in a byte
	struct THaplotype
	{
	public:
		friend class CHaplotypeList;

		/// packed SNP alleles
		UINT8 PackedHaplo[HIBAG_PACKED_UTYPE_MAXNUM];
		/// haplotype frequency
		double Freq;
		/// auxiliary variables, sizeof(THaplotype)=32
		union type_aux
		{
			double OldFreq;  /// old haplotype frequency
			struct type_aux2 {
				float Freq_f32;  /// 32-bit haplotype frequency
				int HLA_allele;  /// the associated HLA allele
			} a2;
		} aux;

		THaplotype();
		THaplotype(double _freq);
		THaplotype(const char *str, double _freq);

		/// get SNP allele, idx starts from ZERO
		UINT8 GetAllele(size_t idx) const;
		/// set SNP allele, idx starts from ZERO
		void SetAllele(size_t idx, UINT8 val);
		/// get a string of "0" and "1" from packed SNP alleles
		string HaploToStr(size_t Length) const;
		/// set packed SNP alleles from a string of "0" and "1"
		void StrToHaplo(const string &str);

	private:
		/// set SNP allele, idx starts from ZERO, without checking
		inline void _SetAllele(size_t idx, UINT8 val);
	};


	/// Haplotype list with an HLA gene and SNP alleles
	class CHaplotypeList
	{
	public:	
		CHaplotypeList();
		CHaplotypeList(const CHaplotypeList &src);
		CHaplotypeList(size_t reserve_num);  // reserve memory for haplotypes
		~CHaplotypeList();

		// assign operator
		CHaplotypeList& operator= (const CHaplotypeList &src);

		/// resize the number of haplotypes, no initialization
		void ResizeHaplo(size_t num);

		/// initialize haplotypes for EM algorithm
		void DoubleHaplos(CHaplotypeList &OutHaplos) const;
		/// initialize haplotypes for EM algorithm with the allele frequency of the new SNP
		void DoubleHaplosInitFreq(CHaplotypeList &OutHaplos, double AFreq) const;
		/// remove rare haplotypes
		void EraseDoubleHaplos(double RareProb, CHaplotypeList &OutHaplos) const;
		/// save frequency to old.frequency, and set current freq. to be 0
		void SaveClearFrequency();
		/// scale the haplotype frequencies by a factor
		void ScaleFrequency(double scale);

		/// the number of haplotypes
		size_t Num_Haplo;
		/// the number of SNP markers
		size_t Num_SNP;
		/// SNP haplotype list
		THaplotype *List;
		/// the number of SNP  haplotypes per HLA allele
		vector<size_t> LenPerHLA;

		/// the start index of an HLA allele
		size_t StartHaploHLA(int hla) const;
		/// the total number of unique HLA alleles
		inline size_t nHLA() const { return LenPerHLA.size(); }

		/// set the auxiliary variable aux.Freq_f32 and aux.HLA_allele
		void SetHaploAux();

	private:
		size_t reserve_size;
		void *base_ptr;

		inline void alloc_mem(size_t num);
	};


	/// A pair of HLA alleles
	struct THLAType
	{
		int Allele1;  //< the first HLA allele
		int Allele2;  //< the second HLA allele
	};


	/// Packed bi-allelic SNP genotype structure: 8 SNPs in a byte
	struct TGenotype
	{
	public:
		friend class CGenotypeList;
		friend class CAlg_EM;
		friend class CAlg_Prediction;

		/// packed SNP genotypes, allele 1
		UINT8 PackedSNP1[HIBAG_PACKED_UTYPE_MAXNUM];
		/// packed SNP genotypes, allele 2
		UINT8 PackedSNP2[HIBAG_PACKED_UTYPE_MAXNUM];
		/// packed SNP genotypes, missing flag
		UINT8 PackedMissing[HIBAG_PACKED_UTYPE_MAXNUM];

		/// the count in the bootstrapped data
		int BootstrapCount;

		/// auxiliary correct HLA type
		THLAType aux_hla_type;
		/// auxiliary integer to make sizeof(TGenotype)=64
		int aux_temp;

		/// constructor
		TGenotype();

		/// get SNP genotype (0, 1, 2) at the specified locus, idx starts from ZERO
		int GetSNP(size_t idx) const;
		/// set SNP genotype (0, 1, 2) at the specified locus, idx starts from ZERO
		void SetSNP(size_t idx, int val);
		/// get a string of SNP genotypes, consisting of "0" or "1"
		string SNPToString(size_t Length) const;
		/// set SNP genotypes by a string of "0" and "1"
		void StringToSNP(const string &str);
		/// export SNPs to a vector of integers
		void SNPToInt(size_t Length, int OutArray[]) const;
		/// import SNPs from an integer vector 'InBase' with 'Index'
		void IntToSNP(size_t Length, const int InBase[], const int Index[]);

		/// compute the Hamming distance between SNPs and H1+H2
		HIBAG_TARGET_CLONES int HammingDistance(size_t Length, const THaplotype &H1, const THaplotype &H2) const;

	protected:
		/// set SNP genotype (0, 1, 2) without checking
		void _SetSNP(size_t idx, int val);
		/// compute the Hamming distance between SNPs and H1+H2 without checking
		ALWAYS_INLINE int _HamDist(size_t Length, const THaplotype &H1,
			const THaplotype &H2) const;
	};


	/// SNP genotype container
	class CSNPGenoMatrix
	{
	public:
		CSNPGenoMatrix();

		/// get SNP genotype of 'IdxSamp' individual and 'IdxSNP' SNP
		const int Get(const int IdxSamp, const int IdxSNP) const;
		/// get the pointer to SNP genotypes of 'IdxSamp' individual
		int *Get(const int IdxSamp);

		/// the total number of SNPs
		int Num_Total_SNP;
		/// the total number of samples
		int Num_Total_Samp;
		/// the pointer to SNP genotypes
		int *pGeno;
	};


	/// A list of genotypes
	class CGenotypeList
	{
	public:
		friend class CVariableSelection;

		CGenotypeList();

		/// add a new SNP
		void AddSNP(int IdxSNP, const CSNPGenoMatrix &SNPMat);
		/// remove the last SNP
		void ReduceSNP();

		/// set all SNPs to missing
		void SetAllMissing();
		/// set missing SNP at a specified position
		void SetMissing(int IdxSNP);

		/// return the total number of samples
		inline int nSamp() const { return List.size(); }

		/// genotype list
		vector<TGenotype> List;
		/// the number of SNPs
		size_t Num_SNP;
	};


	/// A list of HLA types
	class CHLATypeList
	{
	public:
		CHLATypeList();

		/// return the total number of samples
		inline int nSamp() const { return List.size(); }
		/// return the total number of HLA alleles
		inline int Num_HLA_Allele() const { return Str_HLA_Allele.size(); }
		
		/// return how many alleles are the same
		static int Compare(const THLAType &H1, const THLAType &H2);

		/// a list of HLA types
		vector<THLAType> List;
		/// HLA alleles
		vector<string> Str_HLA_Allele;
	};




	// ===================================================================== //
	// ========                      algorithm                      ========

	// the parameter of EM algorithm for estimating haplotype frequencies

	/// The max number of iterations
	extern int EM_MaxNum_Iterations;  // = 500

	/// The reltol convergence tolerance, sqrt(machine.epsilon) by default, used in EM algorithm
	extern double EM_FuncRelTol;  // = sqrt(DBL_EPSILON)


	/// variable sampling
	class CBaseSampling
	{
	public:
		/// the total number of candidate SNPs
		virtual int TotalNum() const = 0;
		/// randomly select 'm_try' SNPs for further searching
		virtual void RandomSelect(int m_try) = 0;
		/// the number of selected SNPs
		virtual int NumOfSelection() const = 0;
		/// remove the 'idx' SNP
		virtual void Remove(int idx) = 0;
		/// remove the selected SNPs
		virtual void RemoveSelection() = 0;
		/// remove the SNPs with flag
		virtual void RemoveFlag() = 0;
		/// get SNP index
		virtual int &operator[] (int idx) = 0;
	};


	/// variable sampling without replacement
	class CSamplingWithoutReplace: public CBaseSampling
	{
	public:
		CSamplingWithoutReplace();
		CBaseSampling *Init(int m_total);

		/// the total number of candidate SNPs
		virtual int TotalNum() const;
		/// randomly select 'm_try' SNPs for further searching
		virtual void RandomSelect(int m_try);
		/// the number of selected SNPs
		virtual int NumOfSelection() const;
		/// remove the 'idx' SNP
		virtual void Remove(int idx);
		/// remove the selected SNPs
		virtual void RemoveSelection();
		/// remove the SNPs with flag
		virtual void RemoveFlag();
		/// get SNP index
		virtual int &operator[] (int idx);

	protected:
		/// saving the SNP indices
		vector<int> _IdxArray;
		/// the number of selected SNPs
		int _m_try;
	};


	/// Expectation Maximization algorithm for estimating haplotype frequencies
	class CAlg_EM
	{
	public:
		CAlg_EM();

		// call PrepareHaplotypes first, and then call PrepareNewSNP

		/// 
		void PrepareHaplotypes(const CHaplotypeList &CurHaplo,
			const CGenotypeList &GenoList, const CHLATypeList &HLAList,
			CHaplotypeList &NextHaplo);

		/// , return true if the new SNP is not monomorphic
		bool PrepareNewSNP(const int NewSNP, const CHaplotypeList &CurHaplo,
			const CSNPGenoMatrix &SNPMat, CGenotypeList &GenoList,
			CHaplotypeList &NextHaplo);

		/// call EM algorithm to estimate haplotype frequencies
		void ExpectationMaximization(CHaplotypeList &NextHaplo);

	protected:
		/// A pair of haplotypes
		struct THaploPair
		{
			bool Flag;       //< if true, the haplotype pair exists in the sample
			THaplotype *H1;  //< the first haplotype
			THaplotype *H2;  //< the second haplotype
			double GenoFreq;     //< genotype frequency

			THaploPair() { Flag = true; H1 = H2 = NULL; }
			THaploPair(THaplotype *i1, THaplotype *i2) { Flag = true; H1 = i1; H2 = i2; }
		};

		/// A list of haplotype pairs
		struct THaploPairList
		{
			int BootstrapCount;           //< the count in the bootstrapped data
			int SampIndex;                //< the sample index in the source data
			vector<THaploPair> PairList;  //< a list of haplotype pairs
		};

		/// pairs of haplotypes for individuals
		vector<THaploPairList> _SampHaploPair;
	};


	/// the algorithm prediction
	class CAlg_Prediction
	{
	public:
		friend class CVariableSelection;
		friend class CAttrBag_Model;

		CAlg_Prediction();

		/// initialize
		/** \param n_hla    the number of unique HLA alleles **/
		void InitPrediction(int n_hla);

		/// initialize the posterior probabilities by setting ZERO
		void InitPostProbBuffer();
		/// initialize the sums of posterior probabilities by setting ZERO
		void InitSumPostProbBuffer();
		/// add the posterior probabilities of a classifier with a weight to the variable for summing up
		void AddProbToSum(double weight);
		/// average over all classifiers
		void NormalizeSumPostProb();

		/// 
		double &IndexPostProb(int H1, int H2);
		/// 
		double &IndexSumPostProb(int H1, int H2);

		/// predict based on SNP profiles and haplotype list,
		//    and save posterior probabilities in '_PostProb'
		void PredictPostProb(const CHaplotypeList &Haplo, const TGenotype &Geno,
			double &SumProb);
		/// the best-guess HLA type from '_PostProb'
		THLAType BestGuess();
		/// the best-guess HLA type from '_SumPostProb'
		THLAType BestGuessEnsemble();

		/// get the number of unique HLA alleles
		inline const int nHLA() const
			{ return _nHLA; }
		/// the posterior probabilities for the current classifier
		inline const vector<double> &PostProb() const
			{ return _PostProb; }
		/// the average posterior probabilities for all classifiers
		inline const vector<double> &SumPostProb() const
			{ return _SumPostProb; }

	protected:
		/// the number of different HLA alleles
		int _nHLA;
		/// plus weight after calling AddProbToSum()
		double _Sum_Weight;
		/// a vector of posterior probabilities
		vector<double> _PostProb;
		/// a vector of posterior probabilities for summing up
		vector<double> _SumPostProb;

		/// the best-guess HLA type based on SNP profiles and haplotype list
		//    without saving posterior probabilities in '_PostProb'
		THLAType _PredBestGuess(const CHaplotypeList &Haplo, const TGenotype &Geno);
		/// the prob of the given HLA type based on SNP profiles and haplotype list
		//    without saving posterior probabilities in '_PostProb'
		double _PredPostProb(const CHaplotypeList &Haplo, const TGenotype &Geno,
			const THLAType &HLA);
	};


	/// variable selection algorithm
	class CVariableSelection
	{
	public:
		CVariableSelection();

		/// initialize
		void InitSelection(CSNPGenoMatrix &snpMat, CHLATypeList &hlaList,
			const int _BootstrapCnt[]);
		/// searching algorithm
		void Search(CBaseSampling &VarSampling, CHaplotypeList &OutHaplo,
			vector<int> &OutSNPIndex, double &Out_Global_Max_OutOfBagAcc,
			int mtry, bool prune, bool verbose, bool verbose_detail);

		/// the number of samples
		inline int nSamp() const { return _SNPMat->Num_Total_Samp; }
		/// the number of SNPs
		inline int nSNP() const { return _SNPMat->Num_Total_SNP; }
		/// the number of unique HLA alleles
		inline int nHLA() const { return _HLAList->Num_HLA_Allele(); }

	protected:
		/// store the genotype matrix
		CSNPGenoMatrix *_SNPMat;
		/// a list of HLA types
		CHLATypeList *_HLAList;
		
		/// a list of genotypes
		CGenotypeList _GenoList;
		/// EM algorithm
		CAlg_EM _EM;
		/// the prediction algorithm
		CAlg_Prediction _Predict;

		/// initialize the haplotype list
		void _InitHaplotype(CHaplotypeList &Haplo);

		/// initialize the evaluation of in-bag and out-of-bag accuracy
		void _Init_EvalAcc(CHaplotypeList &Haplo, CGenotypeList& Geno);
		/// finalize the evaluation of in-bag and out-of-bag accuracy
		void _Done_EvalAcc();
		/// compute the out-of-bag accuracy (the number of correct alleles)
		int _OutOfBagAccuracy(CHaplotypeList &Haplo);
		/// compute the in-bag log likelihood using the haplotypes 'Haplo'
		double _InBagLogLik(CHaplotypeList &Haplo);
	};




	// ===================================================================== //
	// ========                   HIBAG -- model                    ========

	class CAttrBag_Model;

	/// the individual classifier of HIBAG
	class CAttrBag_Classifier
	{
	public:
		friend class CAttrBag_Model;

		CAttrBag_Classifier(CAttrBag_Model &_owner);

		/// initialize the bootstrap sample
		void InitBootstrapCount(int SampCnt[]);
		/// assign the haplotype frequencies
		void Assign(int n_snp, const int snpidx[], const int samp_num[],
			int n_haplo, const double *freq, const int *hla,
			const char * haplo[], double *_acc=NULL);
		/// grow this classifier by adding SNPs
		void Grow(CBaseSampling &VarSampling, int mtry, bool prune,
			bool verbose, bool verbose_detail);

		/// the owner
		inline CAttrBag_Model &Owner() { return *_Owner; }
		/// the number of SNPs
		inline const int nSNP() const { return _SNPIndex.size(); }
		/// the number of haplotyeps
		inline const int nHaplo() const { return _Haplo.Num_Haplo; }
		/// the out-of-bag accuracy
		inline const double OutOfBag_Accuracy() const { return _OutOfBag_Accuracy; }
		/// the SNP selection
		inline const vector<int> &SNPIndex() const { return _SNPIndex; }
		/// the bootstrapped individuals
		inline const vector<int> &BootstrapCount() const { return _BootstrapCount; }
		/// the haplotype list
		inline const CHaplotypeList &Haplotype() const { return _Haplo; }

	protected:
		/// the owner
		CAttrBag_Model *_Owner;
		/// the haplotype list
		CHaplotypeList _Haplo;
		/// the bootstrapped individuals
		vector<int> _BootstrapCount;
		/// the SNP selection
		vector<int> _SNPIndex;
		/// the out-of-bag accuracy
		double _OutOfBag_Accuracy;
	};


	/// HIBAG -- the attribute bagging model
	class CAttrBag_Model
	{
	public:
		friend class CAttrBag_Classifier;

		CAttrBag_Model();

		/// initialize the training model
		void InitTraining(int n_snp, int n_samp, int n_hla);
		/// initialize the training model
		void InitTraining(int n_snp, int n_samp, int *snp_geno, int n_hla, int *H1, int *H2);

		/// get a new individual classifier
		CAttrBag_Classifier *NewClassifierBootstrap();
		/// get a new individual classifier
		CAttrBag_Classifier *NewClassifierAllSamp();

		/// build n individual classifiers with the specified parameters
		void BuildClassifiers(int nclassifier, int mtry, bool prune,
			bool verbose, bool verbose_detail=false);

		/** get the best-guess HLA types
		 *  \param genomat       genotype matrix
		 *  \param n_samp        the number of samples in genomat
		 *  \param vote_method   1: average posterior prob, 2: majority voting
		 *  \param OutH1         the first HLA allele per sample
		 *  \param OutH2         the second HLA allele per sample
		 *  \param OutMaxProb    the posterior prob. of the best-guess HLA genotypes per sample
		 *  \param OutProbArray  the posterior prob. of all HLA genotypes per sample
		 *  \param OutMatching   the sum of prior prob. per sample
		 *  \param ShowInfo      if true, show information
		**/
		void PredictHLA(const int *genomat, int n_samp, int vote_method,
			int OutH1[], int OutH2[], double OutMaxProb[],
			double OutMatching[], double OutProbArray[], bool ShowInfo);

		/// the number of samples
		inline int nSamp() const { return _SNPMat.Num_Total_Samp; }
		/// the number of SNPs
		inline int nSNP() const { return _SNPMat.Num_Total_SNP; }
		/// the number of unique HLA alleles
		inline int nHLA() const { return _HLAList.Num_HLA_Allele(); }

		/// a list of HLA types
		inline const CHLATypeList &HLAList() const
			{ return _HLAList; }
		/// a list of individual classifiers
		inline const vector<CAttrBag_Classifier> &ClassifierList() const
			{ return _ClassifierList; }

	protected:
		/// the SNP genotype matrix
		CSNPGenoMatrix _SNPMat;
		/// a list of HLA alleles
		CHLATypeList _HLAList;
		/// a list of individual classifiers
		vector<CAttrBag_Classifier> _ClassifierList;
		/// variable selection algorithm
		CVariableSelection _VarSelect;
		/// prediction algorithm
		CAlg_Prediction _Predict;

		/// prediction HLA types internally
		void _PredictHLA(const int geno[], const int snp_weight[],
			int vote_method, double &OutMatching);
		/// get weight with respect to the SNP frequencies in the model for missing SNPs
		void _GetSNPWeights(int OutSNPWeight[]);

		void _Init_PredictHLA();
		void _Done_PredictHLA();

	private:
		vector<TGenotype> gpu_geno_buf;
		vector<int> gpu_num_haplo;
	};



	// ===================================================================== //
	// ===================================================================== //

	/// The basic class for progress object
	class CdProgression
	{
	public:
		static const int TotalPercent = 10;
		static const int StepPercent = 10;

		/// The associated information
		std::string Info;

		/// Constructor
		CdProgression();

		/// initialize
		void Init(long TotalCnt, bool ShowInit);
		/// move forward
		bool Forward(long step, bool Show);
		/// show progress information
		virtual void ShowProgress();

        /// Return the current percentile
		inline int Percent() const { return fPercent; }
		/// Return the total number
		inline long Total() const { return fTotal; }
		/// Return the current position
		inline long Current() const { return fCurrent; }

	protected:
		long fTotal;      //< the total number
		long fCurrent;    //< the current number
		int fPercent;     //< the corresponding percent
		clock_t OldTime;  //< the old time point
	};

	/// Progress information
	extern CdProgression Progress;



	// ===================================================================== //

	/// Exceptions for HLA imputation
	class ErrHLA: public std::exception
	{
	public:
		ErrHLA() {};
		ErrHLA(const std::string &msg) { fMessage = msg; }
		ErrHLA(const char *fmt, ...)
			{
				va_list args;
				va_start(args, fmt);
				char buf[1024];
				vsnprintf(buf, sizeof(buf), fmt, args);
				fMessage = buf;
				va_end(args);
			}
		virtual const char *what() const throw() { return fMessage.c_str(); }
		virtual ~ErrHLA() throw() {};

	protected:
		std::string fMessage;
	};



	// ===================================================================== //

	/// the data structure of functions using GPU
	struct TypeGPUExtProc
	{
		/// initialize the internal structure for building a model
		void (*build_init)(int nHLA, int nSample);
		/// finalize the structure for building a model
		void (*build_done)();
		/// initialize bottstrapping
		void (*build_set_bootstrap)(const int oob_cnt[]);
		/// initialize haplotypes and SNPs genotypes
		void (*build_set_haplo_geno)(const THaplotype haplo[], int n_haplo,
			const TGenotype geno[], int n_snp);
		/// calculate the out-of-bag accuracy (the number of correct alleles)
		int (*build_acc_oob)();
		/// calculate the in-bag log likelihood
		double (*build_acc_ib)();

		/// initialize the internal structure for predicting
		//    nHaplo[2*nClassifier]:
		//    nHaplo[0] -- total # of haplotypes
		//    nHaplo[1] -- # of SNPs
		void (*predict_init)(int nHLA, int nClassifier,
			const THaplotype *const pHaplo[], const int nHaplo[]);
		/// finalize the structure for predicting
		void (*predict_done)();
		/// average the posterior probabilities among classifiers for predicting
		void (*predict_avg_prob)(const TGenotype geno[], const double weight[],
			double out_prob[], double out_match[]);
	};

	/// Pointer to the structure of functions using GPU
	extern TypeGPUExtProc *GPUExtProcPtr;

	/// Get CPU information
	extern const char *CPU_Info();
}

#endif /* LIBHLA_H_ */
