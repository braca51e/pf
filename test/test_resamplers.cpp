//#include "UnitTest++.h"
#include <UnitTest++/UnitTest++.h>
#include "resamplers.h"
#include "cf_filters.h"

#define NUMPARTICLES 20
#define DIMSTATE     3
#define DIMINNERMOD  2
#define DIMOBS       1


class MRFixture
{
public:

    // types
    using ssv = Eigen::Matrix<double,DIMSTATE,1>;
    using innerVec = Eigen::Matrix<double,DIMINNERMOD,1>;
    using transMat = Eigen::Matrix<double,DIMINNERMOD,DIMINNERMOD>;
    using arrayVec = std::array<ssv,NUMPARTICLES>;
    using arrayDouble = std::array<double,NUMPARTICLES>;
    using arrayHMMMods = std::array<hmm<DIMINNERMOD,DIMOBS,double>,NUMPARTICLES>;
    using arrayInnerVec = std::array<innerVec,NUMPARTICLES>;
    
    
    // make the resampling object(s)
    mn_resampler<NUMPARTICLES, DIMSTATE,double> m_mr;
    mn_resampler_rbpf<NUMPARTICLES,DIMSTATE,hmm<DIMINNERMOD,DIMOBS,double>,double> m_mr_rbpf_hmm;
    
    // for Test_resampLogWts
    arrayVec    m_vparts;
    arrayDouble m_vw;
    
    // for Test_resampLogWts_RBPF
    innerVec m_initProbDistn1;
    innerVec m_initProbDistn2;
    transMat m_initTransMat1;
    transMat m_initTransMat2;
    arrayHMMMods m_hmms;
    arrayVec m_rbpf_samps;
    arrayDouble m_rbpf_logwts;
    

    MRFixture() : m_initTransMat1(transMat::Zero()), m_initTransMat2(transMat::Zero())
    {
        // for Test_resampLogWts
        // make the first particle have 0 for everything (samples and weights)
        // make all other particles have 1 for samples and -INF for weights
        for(size_t i = 0; i < NUMPARTICLES; ++i){
            if ( i == 0){
                m_vparts[i] = ssv::Constant(0.0);
                m_vw[i] = 0.0;
            }else{
                m_vparts[i] = ssv::Constant(1.0);
                m_vw[i] = -1.0/0.0;
            }
        }

        // for Test_resampLogWts_RBPF
        // make the first particle have 0 for samples and weights,
        //      and will have a model with all weight in the first spot, 
        //      and a transition matrix that keeps everything in that first spot
        
        // make everything BUT the first particle have 1 for samples and -INF for weights
        //      as well as models that have models with all weight in the last spot
        //      and transition matrices that keep it in that last spot 
        for(size_t i = 0; i < DIMINNERMOD; ++i){

            m_initTransMat1(i,0) = 1.0;
            m_initTransMat2(i,DIMINNERMOD-1) = 1.0;
            m_initTransMat1.block(i,1,1,DIMINNERMOD-1) = Eigen::Matrix<double,1,DIMINNERMOD-1>::Zero();
            m_initTransMat2.block(i,0,1,DIMINNERMOD-1) = Eigen::Matrix<double,1,DIMINNERMOD-1>::Zero();
            
            if(i==0){
                m_initProbDistn1(i) = 1.0;
                m_initProbDistn2(i) = 0.0;
            }else if(i == DIMINNERMOD - 1){
                m_initProbDistn1(i) = 0.0;
                m_initProbDistn2(i) = 1.0;
            }else{
                m_initProbDistn1(i) = 0.0;
                m_initProbDistn2(i) = 0.0;
            }
        }
        
        for(size_t i = 0; i < NUMPARTICLES; ++i){
            if(i == 0){
                m_hmms[i] = hmm<DIMINNERMOD,DIMOBS,double>(m_initProbDistn1, m_initTransMat1);
                m_rbpf_samps[i] = ssv::Constant(0.0);
                m_rbpf_logwts[i] = 0.0;
            }else{
                m_hmms[i] = hmm<DIMINNERMOD,DIMOBS,double>(m_initProbDistn2, m_initTransMat2);
                m_rbpf_samps[i] = ssv::Constant(1.0);                
                m_rbpf_logwts[i] = -1.0/0.0;
            }
        }
    }
    
};


TEST_FIXTURE(MRFixture, Test_resampLogWts)
{
    
    m_mr.resampLogWts(m_vparts, m_vw);
    for(unsigned int p = 0; p < NUMPARTICLES; ++p){
        
        CHECK_EQUAL(m_vw[p], 0.0);
        for(unsigned int i = 0; i < DIMSTATE; ++i){
            CHECK_EQUAL(m_vparts[p](i), 0.0);
        }
    }
}

TEST_FIXTURE(MRFixture, Test_resampLogWts_RBPF)
{
    
    // resample... you should only have the first set of things repeated a bunch of times
    m_mr_rbpf_hmm.resampLogWts(m_hmms, m_rbpf_samps, m_rbpf_logwts);
    
    // check all the particles
    for(unsigned int p = 0; p < NUMPARTICLES; ++p){
        
        // log 1 = 0
        CHECK_EQUAL(m_rbpf_logwts[p], 0.0);
        
        // everything should be 0
        for(unsigned int i = 0; i < DIMSTATE; ++i){
            CHECK_EQUAL(m_rbpf_samps[p](i), 0.0);
        }
        
        // the model that keeps everything in the first spot should remain
        innerVec before = m_hmms[p].getFilterVec();
        for(unsigned int i = 0; i < DIMINNERMOD; ++i){
            if(i == 0){
                CHECK_EQUAL(before(i), 1.0); // all weight in the first spot 
            }else{
                CHECK_EQUAL(before(i), 0.0); // all weight in the first spot
            }
        }
        m_hmms[p].update(innerVec::Constant(1.0));
        innerVec after = m_hmms[p].getFilterVec();
        for(unsigned int i = 0; i < DIMINNERMOD; ++i){
            if(i == 0){
                CHECK_EQUAL(after(i), 1.0); // all weight in the first spot 
            }else{
                CHECK_EQUAL(after(i), 0.0); // all weight in the first spot
            }
        }
    }
    
    /**
     * @TODO complete Kalman testing as well
     */
}
