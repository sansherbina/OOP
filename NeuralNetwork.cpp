#pragma once

#include "stdafx.h"
#include "NeuralNetwork.h"
#include "MNist.h"  
#include <malloc.h> 

NeuralNetwork::NeuralNetwork()
{
  Initialize();
}

void NeuralNetwork::Initialize()
{
  
	VectorLayers::iterator it;
	
	for( it=m_Layers.begin(); it<m_Layers.end(); it++ )
	{
		delete *it;
	}
	
	m_Layers.clear();
	
	m_etaLearningRate = .001;
    
	m_cBackprops = 0;
	
}

NeuralNetwork::~NeuralNetwork()
{
	Initialize();
}

void NeuralNetwork::Calculate(double* inputVector, UINT iCount, 
							  double* outputVector , UINT oCount ,
							  std::vector< std::vector< double > >* pNeuronOutputs )
{
	VectorLayers::iterator lit = m_Layers.begin();
	VectorNeurons::iterator nit;
	

	
	if ( lit<m_Layers.end() )  
	{
		nit = (*lit)->m_Neurons.begin();
		int count = 0;
		
		ASSERT( iCount == (*lit)->m_Neurons.size() );  // there should be exactly one neuron per input
		
		while( ( nit < (*lit)->m_Neurons.end() ) && ( count < iCount ) )
		{
			(*nit)->output = inputVector[ count ];
			nit++;
			count++;
		}
	}
	
	
	
	for( lit++; lit<m_Layers.end(); lit++ )
	{
		(*lit)->Calculate();
	}
	
	// load up output vector with results
	
	if ( outputVector != NULL )
	{
		lit = m_Layers.end();
		lit--;
		
		nit = (*lit)->m_Neurons.begin();
		
		for ( int ii=0; ii<oCount; ++ii )
		{
			outputVector[ ii ] = (*nit)->output;
			nit++;
		}
	}
	
	// load up neuron output values with results
	
	if ( pNeuronOutputs != NULL )
	{
		
		if ( pNeuronOutputs->empty() != FALSE )
		{
			
			pNeuronOutputs->clear();  
			
			int ii = 0;
			for( lit=m_Layers.begin(); lit<m_Layers.end(); lit++ )
			{
				std::vector< double > layerOut;
				
				for ( ii=0; ii<(*lit)->m_Neurons.size(); ++ii )
				{
					layerOut.push_back( (*lit)->m_Neurons[ ii ]->output );
				}
				
				pNeuronOutputs->push_back( layerOut);
			}
		}
		else
		{
			
			int ii, jj = 0;
			for( lit=m_Layers.begin(); lit<m_Layers.end(); lit++ )
			{
				for ( ii=0; ii<(*lit)->m_Neurons.size(); ++ii )
				{
					(*pNeuronOutputs)[ jj ][ ii ] = (*lit)->m_Neurons[ ii ]->output ;
				}
				
				++jj;
			}
			
		}
		
	}
	
}


void NeuralNetwork::Backpropagate(double *actualOutput, double *desiredOutput, UINT count,
								  std::vector< std::vector< double > >* pMemorizedNeuronOutputs )
{
	
	ASSERT( ( actualOutput != NULL ) && ( desiredOutput != NULL ) && ( count < 256 ) );
	
	ASSERT( m_Layers.size() >= 2 );  // there must be at least two layers in the net
	
	if ( ( actualOutput == NULL ) || ( desiredOutput == NULL ) || ( count >= 256 ) )
		return;
	

	
	m_cBackprops++;
	
	if ( (m_cBackprops % 10000) == 0 )
	{
		//10000 backprops
		
		PeriodicWeightSanityCheck();
	}
	
	VectorLayers::iterator lit = m_Layers.end() - 1;
	
	std::vector< double > dErr_wrt_dXlast( (*lit)->m_Neurons.size() );
	std::vector< std::vector< double > > differentials;
	
	int iSize = m_Layers.size();
	
	differentials.resize( iSize );
	
	int ii;
	
	for ( ii=0; ii<(*lit)->m_Neurons.size(); ++ii )
	{
		dErr_wrt_dXlast[ ii ] = actualOutput[ ii ] - desiredOutput[ ii ];
	}
	
	differentials[ iSize-1 ] = dErr_wrt_dXlast;  // last one
	
	for ( ii=0; ii<iSize-1; ++ii )
	{
		differentials[ ii ].resize( m_Layers[ii]->m_Neurons.size(), 0.0 );
	}
	
	
	BOOL bMemorized = ( pMemorizedNeuronOutputs != NULL );
	
	lit = m_Layers.end() - 1; 
	
	ii = iSize - 1;
	for ( lit; lit>m_Layers.begin(); lit--)
	{
		if ( bMemorized != FALSE )
		{
			(*lit)->Backpropagate( differentials[ ii ], differentials[ ii - 1 ], 
				&(*pMemorizedNeuronOutputs)[ ii ], &(*pMemorizedNeuronOutputs)[ ii - 1 ], m_etaLearningRate );
		}
		else
		{
			(*lit)->Backpropagate( differentials[ ii ], differentials[ ii - 1 ], 
				NULL, NULL, m_etaLearningRate );
		}
		
		--ii;
	}
	
	
	differentials.clear();
	
}

								  
void NeuralNetwork::PeriodicWeightSanityCheck()
{

	VectorLayers::iterator lit;
	
	for ( lit=m_Layers.begin(); lit<m_Layers.end(); lit++)
	{
		(*lit)->PeriodicWeightSanityCheck();
	}
	
}




void NeuralNetwork::EraseHessianInformation()
{
	
	VectorLayers::iterator lit;
	
	for ( lit=m_Layers.begin(); lit<m_Layers.end(); lit++ )
	{
		(*lit)->EraseHessianInformation();
	}
	
}


void NeuralNetwork::DivideHessianInformationBy( double divisor )
{
	
	VectorLayers::iterator lit;
	
	for ( lit=m_Layers.begin(); lit<m_Layers.end(); lit++ )
	{
		(*lit)->DivideHessianInformationBy( divisor );
	}
	
}


void NeuralNetwork::BackpropagateSecondDervatives( double* actualOutputVector, 
												  double* targetOutputVector, UINT count )
{
	
	ASSERT( ( actualOutputVector != NULL ) && ( targetOutputVector != NULL ) && ( count < 256 ) );
	
	ASSERT( m_Layers.size() >= 2 );  // there must be at least two layers in the net
	
	if ( ( actualOutputVector == NULL ) || ( targetOutputVector == NULL ) || ( count >= 256 ) )
		return;
	
	
	VectorLayers::iterator lit;
	
	lit = m_Layers.end() - 1;  // set to last layer
	
	std::vector< double > d2Err_wrt_dXlast( (*lit)->m_Neurons.size() );
	std::vector< std::vector< double > > differentials;
	
	int iSize = m_Layers.size();
	
	differentials.resize( iSize );
	
	int ii;
	

	lit = m_Layers.end() - 1;  // point to last layer
	
	for ( ii=0; ii<(*lit)->m_Neurons.size(); ++ii )
	{
		d2Err_wrt_dXlast[ ii ] = 1.0;
	}
	
	
	// store Xlast and reserve memory for the remaining vectors stored in differentials
	
	differentials[ iSize-1 ] = d2Err_wrt_dXlast;  // last one
	
	for ( ii=0; ii<iSize-1; ++ii )
	{
		differentials[ ii ].resize( m_Layers[ii]->m_Neurons.size(), 0.0 );
	}
	
	
	lit = m_Layers.end() - 1;
	
	ii = iSize - 1;
	for ( lit; lit>m_Layers.begin(); lit--)
	{
		(*lit)->BackpropagateSecondDerivatives( differentials[ ii ], differentials[ ii - 1 ] );
		
		--ii;
	}
	
	differentials.clear();
}




void NeuralNetwork::Serialize(CArchive &ar)
{
	
}

NNLayer::NNLayer() :
label( _T("") ), m_pPrevLayer( NULL )
{
	Initialize();
}

NNLayer::NNLayer( LPCTSTR str, NNLayer* pPrev /* =NULL */ ) :
label( str ), m_pPrevLayer( pPrev )
{
	Initialize();
}


void NNLayer::Initialize()
{
	VectorWeights::iterator wit;
	VectorNeurons::iterator nit;
	
	for( nit=m_Neurons.begin(); nit<m_Neurons.end(); nit++ )
	{
		delete *nit;
	}
	
	for( wit=m_Weights.begin(); wit<m_Weights.end(); wit++ )
	{
		delete *wit;
	}
	
	m_Weights.clear();
	m_Neurons.clear();
	
	m_bFloatingPointWarning = false;
	
}

NNLayer::~NNLayer()
{
	Initialize();
}

void NNLayer::Calculate()
{
	ASSERT( m_pPrevLayer != NULL );
	
	VectorNeurons::iterator nit;
	VectorConnections::iterator cit;
	
	double dSum;
	
	for( nit=m_Neurons.begin(); nit<m_Neurons.end(); nit++ )
	{
		NNNeuron& n = *(*nit);  // to ease the terminology
		
		cit = n.m_Connections.begin();
		
		ASSERT( (*cit).WeightIndex < m_Weights.size() );
		
		dSum = m_Weights[ (*cit).WeightIndex ]->value;  // weight of the first connection is the bias; neuron is ignored
		
		for ( cit++ ; cit<n.m_Connections.end(); cit++ )
		{
			ASSERT( (*cit).WeightIndex < m_Weights.size() ); 
			ASSERT( (*cit).NeuronIndex < m_pPrevLayer->m_Neurons.size() );
			
			dSum += ( m_Weights[ (*cit).WeightIndex ]->value ) * 
				( m_pPrevLayer->m_Neurons[ (*cit).NeuronIndex ]->output );
		}
		
		n.output = SIGMOID( dSum );
		
	}
	
}



void NNLayer::Backpropagate( std::vector< double >& dErr_wrt_dXn, 
							std::vector< double >& dErr_wrt_dXnm1 , 
							std::vector< double >* thisLayerOutput, 
							std::vector< double >* prevLayerOutput,  
							double etaLearningRate )
{

	ASSERT( dErr_wrt_dXn.size() == m_Neurons.size() );
	ASSERT( m_pPrevLayer != NULL );
	ASSERT( dErr_wrt_dXnm1.size() == m_pPrevLayer->m_Neurons.size() );
	
	int ii, jj;
	UINT kk;
	int nIndex;
	double output;
	
	std::vector< double > dErr_wrt_dYn( m_Neurons.size() );
	
	double* dErr_wrt_dWn = (double*)( _alloca( sizeof(double) *  m_Weights.size() ) );
	
	for ( ii=0; ii<m_Weights.size(); ++ii )
	{
		dErr_wrt_dWn[ ii ] =0.0;
	}
	
	
	VectorNeurons::iterator nit;
	VectorConnections::iterator cit;
	
	
	BOOL bMemorized = ( thisLayerOutput != NULL ) && ( prevLayerOutput != NULL );
	
	
	// calculate dErr_wrt_dYn = F'(Yn) * dErr_wrt_Xn
	
	for ( ii=0; ii<m_Neurons.size(); ++ii )
	{
		ASSERT( ii<dErr_wrt_dYn.size() );
		ASSERT( ii<dErr_wrt_dXn.size() );
		
		if ( bMemorized != FALSE )
		{
			output = (*thisLayerOutput)[ ii ];
		}
		else
		{
			output = m_Neurons[ ii ]->output;
		}
		
		dErr_wrt_dYn[ ii ] = DSIGMOID( output ) * dErr_wrt_dXn[ ii ];
	}
	
	ii = 0;
	for ( nit=m_Neurons.begin(); nit<m_Neurons.end(); nit++ )
	{
		NNNeuron& n = *(*nit);  // for simplifying the terminology
		
		for ( cit=n.m_Connections.begin(); cit<n.m_Connections.end(); cit++ )
		{
			kk = (*cit).NeuronIndex;
			if ( kk == ULONG_MAX )
			{
				output = 1.0;  // this is the bias weight
			}
			else
			{
				ASSERT( kk<m_pPrevLayer->m_Neurons.size() );
				
				if ( bMemorized != FALSE )
				{
					output = (*prevLayerOutput)[ kk ];
				}
				else
				{
					output = m_pPrevLayer->m_Neurons[ kk ]->output;
				}
			}

			ASSERT( ii<dErr_wrt_dYn.size() );
			dErr_wrt_dWn[ (*cit).WeightIndex ] += dErr_wrt_dYn[ ii ] * output;
		}
		
		ii++;
	}

	ii = 0;
	for ( nit=m_Neurons.begin(); nit<m_Neurons.end(); nit++ )
	{
		NNNeuron& n = *(*nit); 
		
		for ( cit=n.m_Connections.begin(); cit<n.m_Connections.end(); cit++ )
		{
			kk=(*cit).NeuronIndex;
			if ( kk != ULONG_MAX )
			{

				nIndex = kk;
				
				ASSERT( nIndex<dErr_wrt_dXnm1.size() );
				ASSERT( ii<dErr_wrt_dYn.size() );
				ASSERT( (*cit).WeightIndex<m_Weights.size() );
				
				dErr_wrt_dXnm1[ nIndex ] += dErr_wrt_dYn[ ii ] * m_Weights[ (*cit).WeightIndex ]->value;
			}
			
		}
		
		ii++;  
		
	}
	
	struct DOUBLE_UNION
	{
		union 
		{
			double dd;
			unsigned __int64 ullong;
		};
	};
	
	DOUBLE_UNION oldValue, newValue;
	
	double dMicron = ::GetPreferences().m_dMicronLimitParameter;
	double epsilon, divisor;
	
	for ( jj=0; jj<m_Weights.size(); ++jj )
	{
		divisor = m_Weights[ jj ]->diagHessian + dMicron ; 
		epsilon = etaLearningRate / divisor;
		oldValue.dd = m_Weights[ jj ]->value;
		newValue.dd = oldValue.dd - epsilon * dErr_wrt_dWn[ jj ];
		
		
		while ( oldValue.ullong != _InterlockedCompareExchange64( (unsigned __int64*)(&m_Weights[ jj ]->value), 
			newValue.ullong, oldValue.ullong ) ) 
		{
			// another thread must have modified the weight.  Obtain its new value, adjust it, and try again
			
			oldValue.dd = m_Weights[ jj ]->value;
			newValue.dd = oldValue.dd - epsilon * dErr_wrt_dWn[ jj ];
		}
		
	}
	
}


void NNLayer::PeriodicWeightSanityCheck()
{
	
	VectorWeights::iterator wit;
	
	for ( wit=m_Weights.begin(); wit<m_Weights.end(); wit++ )
	{
		NNWeight& ww = *(*wit);
		double val = fabs( ww.value );
		
		if ( (val>100.0) && (m_bFloatingPointWarning == false) )
		{
			m_bFloatingPointWarning = true;
		}
	}
}



void NNLayer::EraseHessianInformation()
{
	
	VectorWeights::iterator wit;
	
	for ( wit=m_Weights.begin(); wit<m_Weights.end(); wit++ )
	{
		(*wit)->diagHessian = 0.0;
	}
	
}

void NNLayer::DivideHessianInformationBy(double divisor)
{
	
	VectorWeights::iterator wit;
	double dTemp;
	
	for ( wit=m_Weights.begin(); wit<m_Weights.end(); wit++ )
	{
		dTemp = (*wit)->diagHessian;
		
		if ( dTemp < 0.0 )
		{
			ASSERT ( dTemp >= 0.0 );
			dTemp = 0.0;
		}
		
		(*wit)->diagHessian = dTemp / divisor ;
	}
}


void NNLayer::BackpropagateSecondDerivatives( std::vector< double >& d2ErrIn,
											 std::vector< double >& d2ErrInm1)
{

	
	ASSERT( d2ErrIn.size() == m_Neurons.size() );
	ASSERT( m_pPrevLayer != NULL );
	ASSERT( d2ErrInm1.size() == m_pPrevLayer->m_Neurons.size() );

	int ii, jj;
	UINT kk;
	int nIndex;
	double output;
	double dTemp;
		
	std::vector< double > d2Err_wrt_dYn( m_Neurons.size() );
	
	double* d2Err_wrt_dWn = (double*)( _alloca( sizeof(double) *  m_Weights.size() ) );
	
	for ( ii=0; ii<m_Weights.size(); ++ii )
	{
		d2Err_wrt_dWn[ ii ] =0.0;
	}

	VectorNeurons::iterator nit;
	VectorConnections::iterator cit;

	
	// calculate d2Err_wrt_dYn = ( F'(Yn) )^2 * dErr_wrt_Xn (where dErr_wrt_Xn is actually a second derivative )
	
	for ( ii=0; ii<m_Neurons.size(); ++ii )
	{
		ASSERT( ii<d2Err_wrt_dYn.size() );
		ASSERT( ii<d2ErrIn.size() );
		
		output = m_Neurons[ ii ]->output;
		
		dTemp = DSIGMOID( output ) ;
		d2Err_wrt_dYn[ ii ] = d2ErrIn[ ii ] * dTemp * dTemp;
	}

	ii = 0;
	for ( nit=m_Neurons.begin(); nit<m_Neurons.end(); nit++ )
	{
		NNNeuron& n = *(*nit);  // for simplifying the terminology
		
		for ( cit=n.m_Connections.begin(); cit<n.m_Connections.end(); cit++ )
		{
			kk = (*cit).NeuronIndex;
			if ( kk == ULONG_MAX )
			{
				output = 1.0;  // this is the bias connection; implied neuron output of "1"
			}
			else
			{
				ASSERT( kk<m_pPrevLayer->m_Neurons.size() );
				
				output = m_pPrevLayer->m_Neurons[ kk ]->output;
			}
			ASSERT( ii<d2Err_wrt_dYn.size() );
			d2Err_wrt_dWn[ (*cit).WeightIndex ] += d2Err_wrt_dYn[ ii ] * output * output ;
		}
		
		ii++;
	}

	ii = 0;
	for ( nit=m_Neurons.begin(); nit<m_Neurons.end(); nit++ )
	{
		NNNeuron& n = *(*nit);  // for simplifying the terminology
		
		for ( cit=n.m_Connections.begin(); cit<n.m_Connections.end(); cit++ )
		{
			kk=(*cit).NeuronIndex;
			if ( kk != ULONG_MAX )
			{

				nIndex = kk;
				
				ASSERT( nIndex<d2Err_wrt_dXnm1.size() );
				ASSERT( ii<d2Err_wrt_dYn.size() );
				ASSERT( (*cit).WeightIndex<m_Weights.size() );
				
				dTemp = m_Weights[ (*cit).WeightIndex ]->value ; 
				
				d2Err_wrt_dXnm1[ nIndex ] += d2Err_wrt_dYn[ ii ] * dTemp * dTemp ;
			}
			
		}
		
		ii++;  // ii tracks the neuron iterator
		
	}
	
	struct DOUBLE_UNION
	{
		union 
		{
			double dd;
			unsigned __int64 ullong;
		};
	};
	
	DOUBLE_UNION oldValue, newValue;

	for ( jj=0; jj<m_Weights.size(); ++jj )
	{
		oldValue.dd = m_Weights[ jj ]->diagHessian;
		newValue.dd = oldValue.dd + d2Err_wrt_dWn[ jj ];
		
		while ( oldValue.ullong != _InterlockedCompareExchange64( (unsigned __int64*)(&m_Weights[ jj ]->diagHessian), 
			newValue.ullong, oldValue.ullong ) ) 
		{

			oldValue.dd = m_Weights[ jj ]->diagHessian;
			newValue.dd = oldValue.dd + d2Err_wrt_dWn[ jj ];
		}
		
	}
	
}



void NNLayer::Serialize(CArchive &ar)
{
	VectorNeurons::iterator nit;
	VectorWeights::iterator wit;
	VectorConnections::iterator cit;
	
	int ii, jj;
	
	if (ar.IsStoring())
	{
		ar.WriteString( label.c_str() );
		ar.WriteString( _T("\r\n") );  
		ar << m_Neurons.size();
		ar << m_Weights.size();
		
		
		
		for ( nit=m_Neurons.begin(); nit<m_Neurons.end(); nit++ )
		{
			NNNeuron& n = *(*nit);
			ar.WriteString( n.label.c_str() );
			ar.WriteString( _T("\r\n") );
			ar << n.m_Connections.size();
			
			for ( cit=n.m_Connections.begin(); cit<n.m_Connections.end(); cit++ )
			{
				ar << (*cit).NeuronIndex;
				ar << (*cit).WeightIndex;
			}
		}
		
		for ( wit=m_Weights.begin(); wit<m_Weights.end(); wit++ )
		{
			ar.WriteString( (*wit)->label.c_str() );
			ar.WriteString( _T("\r\n") );
			ar << (*wit)->value;
		}
		
		
	}
	else
	{
	
		CString str;
		ar.ReadString( str );
		
		label = str;
		
		int iNumNeurons, iNumWeights, iNumConnections;
		double value;
		
		NNNeuron* pNeuron;
		NNWeight* pWeight;
		NNConnection conn;
		
		ar >> iNumNeurons;
		ar >> iNumWeights;
		
		for ( ii=0; ii<iNumNeurons; ++ii )
		{
			ar.ReadString( str );
			pNeuron = new NNNeuron( (LPCTSTR)str );
			m_Neurons.push_back( pNeuron );
			
			ar >> iNumConnections;
			
			for ( jj=0; jj<iNumConnections; ++jj )
			{
				ar >> conn.NeuronIndex;
				ar >> conn.WeightIndex;
				
				pNeuron->AddConnection( conn );
			}
		}
		
		for ( jj=0; jj<iNumWeights; ++jj )
		{
			ar.ReadString( str );
			ar >> value;
			
			pWeight = new NNWeight( (LPCTSTR)str, value );
			m_Weights.push_back( pWeight );
		}
		
	}
	
}



NNWeight::NNWeight() : 
label( _T("") ),
value( 0.0 ), diagHessian( 0.0 )
{
	Initialize();
}

NNWeight::NNWeight( LPCTSTR str, double val /* =0.0 */ ) :
label( str ),
value( val ), diagHessian( 0.0 )
{
	Initialize();
}


void NNWeight::Initialize()
{
	
}

NNWeight::~NNWeight()
{
	
}

NNNeuron::NNNeuron() :
label( _T("") ), output( 0.0 )
{
	Initialize();
}

NNNeuron::NNNeuron( LPCTSTR str ) : 
label( str ), output( 0.0 )
{
	Initialize();
}


void NNNeuron::Initialize()
{
	m_Connections.clear();
}

NNNeuron::~NNNeuron()
{
	Initialize();
}


void NNNeuron::AddConnection( UINT iNeuron, UINT iWeight )
{
	m_Connections.push_back( NNConnection( iNeuron, iWeight ) );
}


void NNNeuron::AddConnection( NNConnection const & conn )
{
	m_Connections.push_back( conn );
}













