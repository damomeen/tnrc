/*
 *  This file is part of phosphorus-g2mpls.
 *
 *  Copyright (C) 2006, 2007, 2008, 2009 Nextworks s.r.l.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Authors:
 *
 *  Giacomo Bernini       (Nextworks s.r.l.) <g.bernini_at_nextworks.it>
 *  Gino Carrozzo         (Nextworks s.r.l.) <g.carrozzo_at_nextworks.it>
 *  Nicola Ciulli         (Nextworks s.r.l.) <n.ciulli_at_nextworks.it>
 *  Giodi Giorgi          (Nextworks s.r.l.) <g.giorgi_at_nextworks.it>
 *  Francesco Salvestrini (Nextworks s.r.l.) <f.salvestrini_at_nextworks.it>
 */


#ifndef FSMGEN_H
#define FSMGEN_H
//
// This file has been generated automatically, DO NOT CHANGE !
//

#ifndef FSMGEN_UTILITY
#define FSMGEN_UTILITY

#include <iostream>
#include <stdio.h>
#include <string>
#include <map>
#include <list>
#include <stdlib.h>

/*****************************************************************************/
/*                          Utility - Matrix class                           */
/*****************************************************************************/

template <class ROW, class COLUMN, class DATA>
class Matrix {
public:
	Matrix() { }
	~Matrix() { }

	//friend class Fsm;

	// Copy operator
	Matrix(const Matrix<ROW, COLUMN, DATA>& m) { matrix_ = m.matrix_; }

	//
	// Type definitions
	//
	typedef ROW *          rowIter;
	typedef const ROW *    const_rowIter;
	typedef COLUMN *       colIter;
	typedef const COLUMN * const_colIter;
	typedef DATA *         dataIter;
	typedef const DATA *   const_dataIter;

	//
        // Iterator support
	//
	rowIter begin(void);
	const_rowIter begin(void) const;
	rowIter end(void);
	const_rowIter end(void) const;
	rowIter next(rowIter rowIt);
	const_rowIter next(const_rowIter rowIt) const;
	colIter begin(rowIter rowIt);
	const_colIter begin(const_rowIter rowIt) const;
	colIter end(rowIter rowIt);
	const_colIter end(const_rowIter rowIt) const;
	colIter next(rowIter rowIt, colIter colIt);
	const_colIter next(const_rowIter rowIt, const_colIter colIt) const;


	// Return the number of deleted cells within the row
	size_t removeRow(const ROW& row);

	// Return the number of deleted cells within the column
	size_t removeCol(const COLUMN& column);

	// Return the number (1 or 0) of deleted data for this pair row/column
	size_t remove(const ROW& row, const COLUMN& column);

	// Return the number of deleted data
	size_t remove(const DATA& data);

	bool remove(dataIter dIt);

	void insert(const ROW& row, const COLUMN& column, const DATA& data);

	dataIter find(const ROW& row, const COLUMN& column);

	// Return the number of data
	size_t size(void) const;

	bool empty(void) const;

	// Assignment operator
	Matrix<ROW, COLUMN, DATA>& operator=(const Matrix<ROW,COLUMN,DATA>& o);

	std::map<COLUMN, DATA>& operator[](const ROW s);

	friend std::ostream& operator<<(std::ostream& s, const Matrix& m) {
		typename std::map<ROW,
			std::map<COLUMN, DATA> >::const_iterator rIt;
		for (rIt = m.matrix_.begin(); rIt != m.matrix_.end(); rIt++) {
			s << "[" << (*rIt).first << "] ";

			typename std::map<COLUMN, DATA>::const_iterator cIt;
			for (cIt = (*rIt).second.begin();
			     cIt != (*rIt).second.end();
			     cIt++) {
				if (cIt != (*rIt).second.begin()) {
					s << " - ";
				}
				s << (*cIt).first << "/" << (*cIt).second;
			}

			s << std::endl;
		}
		return s;
	}

private:
	std::map< ROW, std::map<COLUMN, DATA> > matrix_;
};


template <class ROW, class COLUMN, class DATA>
size_t
Matrix<ROW, COLUMN, DATA>::removeRow(const ROW& row)
{
	size_t count;
	count = 0;
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;

	rIt = matrix_.find(row);
	if (rIt != matrix_.end()) {
		//std::cout << "Removed row: " << (*rIt).first
		//	  << std::endl;
		count += (*rIt).second.size();
		matrix_.erase(rIt);
	}

	return count;
}

template <class ROW, class COLUMN, class DATA>
size_t
Matrix<ROW, COLUMN, DATA>::removeCol(const COLUMN& column)
{
	size_t count;
	count = 0;
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;

	for (rIt = matrix_.begin(); rIt != matrix_.end(); rIt++) {
		typename std::map<COLUMN, DATA>::iterator cIt;

		cIt = (*rIt).second.find(column);
		if (cIt != (*rIt).second.end()) {
			//std::cout << "Removed col: " << (*cIt).first
			//	  << std::endl;
			(*rIt).second.erase(cIt);
			count++;
		}
	}

	return count;
}

template <class ROW, class COLUMN, class DATA>
size_t
Matrix<ROW, COLUMN, DATA>::remove(const ROW& row, const COLUMN& column)
{
	size_t count;
	count = 0;
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;

	rIt = matrix_.find(row);
	if (rIt != matrix_.end()) {
		//std::cout << "Row: " << (*rIt).first << std::endl;
		typename std::map<COLUMN, DATA>::iterator cIt;

		cIt = (*rIt).second.find(column);
		if (cIt != (*rIt).second.end()) {
			//std::cout << "  Col: " << (*cIt).first
			//	  << std::endl
			//	  << "    Removed data: "
			//	  << (*cIt).second << std::endl;
			(*rIt).second.erase(cIt);
			count++;
		}
	}

	return count;
}

template <class ROW, class COLUMN, class DATA>
size_t
Matrix<ROW, COLUMN, DATA>::remove(const DATA& data)
{
	size_t count;
	count = 0;
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;

	for (rIt = matrix_.begin(); rIt != matrix_.end(); rIt++) {
		typename std::map<COLUMN, DATA>::iterator cIt;
		cIt = (*rIt).second.begin();
		while (cIt != (*rIt).second.end()) {
			if ((*cIt).second == data) {
				(*rIt).second.erase(cIt++);
				count++;
				continue;
			}
			cIt++;
		}
	}

	return count;
}

template <class ROW, class COLUMN, class DATA>
bool
Matrix<ROW, COLUMN, DATA>::remove(dataIter dIt)
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;

	for (rIt = matrix_.begin(); rIt != matrix_.end(); rIt++) {
		typename std::map<COLUMN, DATA>::iterator cIt;
		for (cIt = (*rIt).second.begin();
		     cIt != (*rIt).second.end();
		     cIt++) {
			if (dIt == &(*cIt).second) {
				(*rIt).second.erase(cIt);
				return true;
			}
		}
	}

	return false;
}

template <class ROW, class COLUMN, class DATA>
void
Matrix<ROW, COLUMN, DATA>::insert(const ROW& row,
				  const COLUMN& column,
				  const DATA& data)
{
	matrix_[row][column] = data;
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::dataIter
Matrix<ROW, COLUMN, DATA>::find(const ROW& row, const COLUMN& column)
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;

	rIt = matrix_.find(row);
	if (rIt != matrix_.end()) {
		//std::cout << "Row: " << (*rIt).first << std::endl;
		typename std::map<COLUMN, DATA>::iterator cIt;

		cIt = (*rIt).second.find(column);
		if (cIt != (*rIt).second.end()) {
			//std::cout << "  Col: " << (*cIt).first
			//	  << std::endl
			//	  << "    Removed data: "
			//	  << (*cIt).second << std::endl;
			return (dataIter)&((*cIt).second);
		}
	}

	return (dataIter)0;
}

template <class ROW, class COLUMN, class DATA>
size_t
Matrix<ROW, COLUMN, DATA>::size(void) const
{
	size_t count;
	count = 0;
	typename std::map< ROW, std::map<COLUMN, DATA> >::const_iterator rIt;

	for (rIt = matrix_.begin(); rIt != matrix_.end(); rIt++) {
		count += (*rIt).second.size();
	}

	return count;
}

template <class ROW, class COLUMN, class DATA>
bool
Matrix<ROW, COLUMN, DATA>::empty(void) const
{
	return size() ? false : true;
}

template <class ROW, class COLUMN, class DATA>
Matrix<ROW, COLUMN, DATA>&
Matrix<ROW, COLUMN, DATA>::operator=(const Matrix<ROW, COLUMN, DATA>& orig)
{
	if(this != &orig) {
		typename std::map< ROW, std::map<COLUMN,DATA> >::iterator rIt;
		for (rIt=matrix_.begin(); rIt!=matrix_.end(); rIt++) {
			(*rIt).second.clear();
		}
		matrix_.clear();

		matrix_ = orig.matrix_;
	}
	return *this;
}

template <class ROW, class COLUMN, class DATA>
std::map<COLUMN, DATA>&
Matrix<ROW, COLUMN, DATA>::operator[](const ROW s)
{
	return matrix_[s];
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::rowIter
Matrix<ROW, COLUMN, DATA>::begin(void)
{
	return (rowIter)&((*(matrix_.begin())).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::const_rowIter
Matrix<ROW, COLUMN, DATA>::begin(void) const
{
	return (const_rowIter)&((*(matrix_.begin())).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::rowIter
Matrix<ROW, COLUMN, DATA>::end(void)
{
	return (rowIter)&((*(matrix_.end())).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::const_rowIter
Matrix<ROW, COLUMN, DATA>::end(void) const
{
	return (const_rowIter)&((*(matrix_.end())).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::rowIter
Matrix<ROW, COLUMN, DATA>::next(rowIter rowIt)
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;
	rIt = matrix_.find(*rowIt);
	rIt++;
	return (rowIter)&((*rIt).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::const_rowIter
Matrix<ROW, COLUMN, DATA>::next(const_rowIter rowIt) const
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::const_iterator rIt;
	rIt = matrix_.find(*rowIt);
	rIt++;
	return (const_rowIter)&((*rIt).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::colIter
Matrix<ROW, COLUMN, DATA>::begin(rowIter rowIt)
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;
	typename std::map<COLUMN, DATA>::iterator cIt;
	rIt = matrix_.find(*rowIt);
	cIt = (*rIt).second.begin();
	return (colIter)&((*cIt).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::const_colIter
Matrix<ROW, COLUMN, DATA>::begin(const_rowIter rowIt) const
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::const_iterator rIt;
	typename std::map<COLUMN, DATA>::const_iterator  cIt;
	rIt = matrix_.find(*rowIt);
	cIt = (*rIt).second.begin();
	return (const_colIter)&((*cIt).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::colIter
Matrix<ROW, COLUMN, DATA>::end(rowIter rowIt)
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;
	typename std::map<COLUMN, DATA>::iterator cIt;
	rIt = matrix_.find(*rowIt);
	cIt = (*rIt).second.end();
	return (colIter)&((*cIt).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::const_colIter
Matrix<ROW, COLUMN, DATA>::end(const_rowIter rowIt) const
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::const_iterator rIt;
	typename std::map<COLUMN, DATA>::const_iterator  cIt;
	rIt = matrix_.find(*rowIt);
	cIt = (*rIt).second.end();
	return (const_colIter)&((*cIt).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::colIter
Matrix<ROW, COLUMN, DATA>::next(rowIter rowIt, colIter colIt)
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::iterator rIt;
	typename std::map<COLUMN, DATA>::iterator cIt;
	rIt = matrix_.find(*rowIt);
	cIt = (*rIt).second.find(*colIt);
	cIt++;
	return (colIter)&((*cIt).first);
}

template <class ROW, class COLUMN, class DATA>
typename Matrix<ROW, COLUMN, DATA>::const_colIter
Matrix<ROW, COLUMN, DATA>::next(const_rowIter rowIt,
				const_colIter colIt) const
{
	typename std::map< ROW, std::map<COLUMN, DATA> >::const_iterator rIt;
	typename std::map<COLUMN, DATA>::const_iterator  cIt;
	rIt = matrix_.find(*rowIt);
	cIt = (*rIt).second.find(*colIt);
	cIt++;
	return (const_colIter)&((*cIt).first);
}
#endif // FSMGEN_UTILITY

namespace fsm {


#ifndef NAMESPACE_BASE_TNRC
#define NAMESPACE_BASE_TNRC

#include <iostream>
#include <stdio.h>
#include <string>
#include <map>
#include <list>
#include <stdlib.h>

/*****************************************************************************/
/*                             Finite State Machine                          */
/*****************************************************************************/
	/*************************************/
	/*    Finite State Machine - Core    */
	/*************************************/
	namespace base_TNRC {

#define STATE_NAME(x)              \
  ((x == fsm::base_TNRC::Fsm::TNRC_Closing ? "Closing" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_Incomplete ? "Incomplete" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_Creating ? "Creating" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_Up ? "Up" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_Down ? "Down" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_Dismissed ? "Dismissed" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_virt_Closing ? "virt_Closing" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_virt_Incomplete ? "virt_Incomplete" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_virt_Creating ? "virt_Creating" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_virt_Up ? "virt_Up" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_virt_Down ? "virt_Down" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_virt_Dismissed ? "virt_Dismissed" :     \
		       "Unknown"))))))))))))))))))))))))

#define EVENT_NAME(x)              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionRetryTimer ? "evAtomicActionRetryTimer" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evActionDestroy ? "evActionDestroy" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evActionPending ? "evActionPending" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionDownTimeout ? "evAtomicActionDownTimeout" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evActionNotification ? "evActionNotification" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionOk ? "evAtomicActionOk" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionNext ? "evAtomicActionNext" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evActionEndUp ? "evActionEndUp" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evActionEndDown ? "evActionEndDown" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionTimeout ? "evAtomicActionTimeout" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evEqptDown ? "evEqptDown" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evActionRollback ? "evActionRollback" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionKo ? "evAtomicActionKo" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionRetry ? "evAtomicActionRetry" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionIncomplete ? "evAtomicActionIncomplete" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evAtomicActionAbort ? "evAtomicActionAbort" :              \
  ((x == fsm::base_TNRC::Fsm::TNRC_evActionCreate ? "evActionCreate" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_AtomicActionRetryTimer ? "AtomicActionRetryTimer" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_ActionDestroy ? "ActionDestroy" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_AtomicActionDownTimeout ? "AtomicActionDownTimeout" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_ActionNotification ? "ActionNotification" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_AtomicActionOk ? "AtomicActionOk" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_AtomicActionTimeout ? "AtomicActionTimeout" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_EqptDown ? "EqptDown" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_ActionRollback ? "ActionRollback" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_AtomicActionKo ? "AtomicActionKo" : \
  ((x == fsm::base_TNRC::Fsm::TNRC_ActionCreate ? "ActionCreate" :      \
		       "Unknown"))))))))))))))))))))))))))))))))))))))))))))))))))))))

#define DUMP(level, threshold, tag, str)				\
		if ((level) <= threshold) {				\
			std::cout << "[" << getpid() << "]"		\
				  << "[" << __FUNCTION__ << "]: "	\
				  << "[" << tag << "] "			\
				  << str				\
				  << std::endl;				\
		}


                enum nextEvFor_AtomicActionRetryTimer_t {
		        TNRC_from_AtomicActionRetryTimer_to_InvalidEvent = 0,
			TNRC_from_AtomicActionRetryTimer_to_evAtomicActionRetryTimer,

		};

                enum nextEvFor_ActionDestroy_t {
		        TNRC_from_ActionDestroy_to_InvalidEvent = 0,
			TNRC_from_ActionDestroy_to_evActionDestroy,
			TNRC_from_ActionDestroy_to_evActionPending,

		};

                enum nextEvFor_AtomicActionDownTimeout_t {
		        TNRC_from_AtomicActionDownTimeout_to_InvalidEvent = 0,
			TNRC_from_AtomicActionDownTimeout_to_evAtomicActionDownTimeout,

		};

                enum nextEvFor_ActionNotification_t {
		        TNRC_from_ActionNotification_to_InvalidEvent = 0,
			TNRC_from_ActionNotification_to_evActionNotification,

		};

                enum nextEvFor_AtomicActionOk_t {
		        TNRC_from_AtomicActionOk_to_InvalidEvent = 0,
			TNRC_from_AtomicActionOk_to_evAtomicActionOk,
			TNRC_from_AtomicActionOk_to_evAtomicActionNext,
			TNRC_from_AtomicActionOk_to_evActionEndUp,
			TNRC_from_AtomicActionOk_to_evActionEndDown,

		};

                enum nextEvFor_AtomicActionTimeout_t {
		        TNRC_from_AtomicActionTimeout_to_InvalidEvent = 0,
			TNRC_from_AtomicActionTimeout_to_evAtomicActionTimeout,

		};

                enum nextEvFor_EqptDown_t {
		        TNRC_from_EqptDown_to_InvalidEvent = 0,
			TNRC_from_EqptDown_to_evEqptDown,

		};

                enum nextEvFor_ActionRollback_t {
		        TNRC_from_ActionRollback_to_InvalidEvent = 0,
			TNRC_from_ActionRollback_to_evActionRollback,

		};

                enum nextEvFor_AtomicActionKo_t {
		        TNRC_from_AtomicActionKo_to_InvalidEvent = 0,
			TNRC_from_AtomicActionKo_to_evAtomicActionKo,
			TNRC_from_AtomicActionKo_to_evAtomicActionRetry,
			TNRC_from_AtomicActionKo_to_evAtomicActionIncomplete,
			TNRC_from_AtomicActionKo_to_evAtomicActionAbort,

		};

                enum nextEvFor_ActionCreate_t {
		        TNRC_from_ActionCreate_to_InvalidEvent = 0,
			TNRC_from_ActionCreate_to_evActionCreate,

		};


		class State {
		public:
			State(std::string name = "Base state") :
				name_(name) { }
			virtual ~State(void) { }

			std::string name(void) { return name_; }

			// On event

			virtual bool evAtomicActionRetryTimer(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionRetryTimer'"));
			}

			virtual bool evActionDestroy(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evActionDestroy'"));
			}

			virtual bool evActionPending(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evActionPending'"));
			}

			virtual bool evAtomicActionDownTimeout(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionDownTimeout'"));
			}

			virtual bool evActionNotification(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evActionNotification'"));
			}

			virtual bool evAtomicActionOk(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionOk'"));
			}

			virtual bool evAtomicActionNext(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionNext'"));
			}

			virtual bool evActionEndUp(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evActionEndUp'"));
			}

			virtual bool evActionEndDown(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evActionEndDown'"));
			}

			virtual bool evAtomicActionTimeout(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionTimeout'"));
			}

			virtual bool evEqptDown(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evEqptDown'"));
			}

			virtual bool evActionRollback(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evActionRollback'"));
			}

			virtual bool evAtomicActionKo(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionKo'"));
			}

			virtual bool evAtomicActionRetry(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionRetry'"));
			}

			virtual bool evAtomicActionIncomplete(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionIncomplete'"));
			}

			virtual bool evAtomicActionAbort(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evAtomicActionAbort'"));
			}

			virtual bool evActionCreate(void * context) {
				throw(std::string("Undefined transition on event "
				      "'evActionCreate'"));
			}

			virtual nextEvFor_AtomicActionRetryTimer_t AtomicActionRetryTimer(void * context) {
				throw(std::string("Undefined transition on event "
				      "'AtomicActionRetryTimer'"));
			}

			virtual nextEvFor_ActionDestroy_t ActionDestroy(void * context) {
				throw(std::string("Undefined transition on event "
				      "'ActionDestroy'"));
			}

			virtual nextEvFor_AtomicActionDownTimeout_t AtomicActionDownTimeout(void * context) {
				throw(std::string("Undefined transition on event "
				      "'AtomicActionDownTimeout'"));
			}

			virtual nextEvFor_ActionNotification_t ActionNotification(void * context) {
				throw(std::string("Undefined transition on event "
				      "'ActionNotification'"));
			}

			virtual nextEvFor_AtomicActionOk_t AtomicActionOk(void * context) {
				throw(std::string("Undefined transition on event "
				      "'AtomicActionOk'"));
			}

			virtual nextEvFor_AtomicActionTimeout_t AtomicActionTimeout(void * context) {
				throw(std::string("Undefined transition on event "
				      "'AtomicActionTimeout'"));
			}

			virtual nextEvFor_EqptDown_t EqptDown(void * context) {
				throw(std::string("Undefined transition on event "
				      "'EqptDown'"));
			}

			virtual nextEvFor_ActionRollback_t ActionRollback(void * context) {
				throw(std::string("Undefined transition on event "
				      "'ActionRollback'"));
			}

			virtual nextEvFor_AtomicActionKo_t AtomicActionKo(void * context) {
				throw(std::string("Undefined transition on event "
				      "'AtomicActionKo'"));
			}

			virtual nextEvFor_ActionCreate_t ActionCreate(void * context) {
				throw(std::string("Undefined transition on event "
				      "'ActionCreate'"));
			}


			// After event from state

			virtual void after_AtomicActionOk_from_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionOk' from state "
				      "'Closing'"));
			}

			virtual void after_evActionEndDown_from_virt_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evActionEndDown' from state "
				      "'virt_Closing'"));
			}

			virtual void after_AtomicActionDownTimeout_from_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionDownTimeout' from state "
				      "'Closing'"));
			}

			virtual void after_evAtomicActionDownTimeout_from_virt_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionDownTimeout' from state "
				      "'virt_Closing'"));
			}

			virtual void after_AtomicActionKo_from_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionKo' from state "
				      "'Closing'"));
			}

			virtual void after_evAtomicActionRetry_from_virt_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionRetry' from state "
				      "'virt_Closing'"));
			}

			virtual void after_evAtomicActionAbort_from_virt_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionAbort' from state "
				      "'virt_Closing'"));
			}

			virtual void after_AtomicActionRetryTimer_from_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionRetryTimer' from state "
				      "'Closing'"));
			}

			virtual void after_evAtomicActionRetryTimer_from_virt_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionRetryTimer' from state "
				      "'virt_Closing'"));
			}

			virtual void after_evAtomicActionNext_from_virt_Closing(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionNext' from state "
				      "'virt_Closing'"));
			}

			virtual void after_ActionRollback_from_Incomplete(void * context) {
				throw(std::string("Undefined transition after event "
				      "'ActionRollback' from state "
				      "'Incomplete'"));
			}

			virtual void after_evActionRollback_from_virt_Incomplete(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evActionRollback' from state "
				      "'virt_Incomplete'"));
			}

			virtual void after_EqptDown_from_Incomplete(void * context) {
				throw(std::string("Undefined transition after event "
				      "'EqptDown' from state "
				      "'Incomplete'"));
			}

			virtual void after_evEqptDown_from_virt_Incomplete(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evEqptDown' from state "
				      "'virt_Incomplete'"));
			}

			virtual void after_AtomicActionKo_from_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionKo' from state "
				      "'Creating'"));
			}

			virtual void after_evAtomicActionKo_from_virt_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionKo' from state "
				      "'virt_Creating'"));
			}

			virtual void after_ActionDestroy_from_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'ActionDestroy' from state "
				      "'Creating'"));
			}

			virtual void after_evActionPending_from_virt_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evActionPending' from state "
				      "'virt_Creating'"));
			}

			virtual void after_EqptDown_from_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'EqptDown' from state "
				      "'Creating'"));
			}

			virtual void after_evEqptDown_from_virt_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evEqptDown' from state "
				      "'virt_Creating'"));
			}

			virtual void after_AtomicActionOk_from_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionOk' from state "
				      "'Creating'"));
			}

			virtual void after_evActionEndUp_from_virt_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evActionEndUp' from state "
				      "'virt_Creating'"));
			}

			virtual void after_evAtomicActionNext_from_virt_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionNext' from state "
				      "'virt_Creating'"));
			}

			virtual void after_evActionDestroy_from_virt_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evActionDestroy' from state "
				      "'virt_Creating'"));
			}

			virtual void after_evAtomicActionIncomplete_from_virt_Creating(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionIncomplete' from state "
				      "'virt_Creating'"));
			}

			virtual void after_ActionNotification_from_Up(void * context) {
				throw(std::string("Undefined transition after event "
				      "'ActionNotification' from state "
				      "'Up'"));
			}

			virtual void after_evActionNotification_from_virt_Up(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evActionNotification' from state "
				      "'virt_Up'"));
			}

			virtual void after_ActionDestroy_from_Up(void * context) {
				throw(std::string("Undefined transition after event "
				      "'ActionDestroy' from state "
				      "'Up'"));
			}

			virtual void after_evActionDestroy_from_virt_Up(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evActionDestroy' from state "
				      "'virt_Up'"));
			}

			virtual void after_ActionCreate_from_Down(void * context) {
				throw(std::string("Undefined transition after event "
				      "'ActionCreate' from state "
				      "'Down'"));
			}

			virtual void after_evActionCreate_from_virt_Down(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evActionCreate' from state "
				      "'virt_Down'"));
			}

			virtual void after_AtomicActionTimeout_from_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionTimeout' from state "
				      "'Dismissed'"));
			}

			virtual void after_evAtomicActionTimeout_from_virt_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionTimeout' from state "
				      "'virt_Dismissed'"));
			}

			virtual void after_AtomicActionKo_from_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionKo' from state "
				      "'Dismissed'"));
			}

			virtual void after_evAtomicActionKo_from_virt_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionKo' from state "
				      "'virt_Dismissed'"));
			}

			virtual void after_AtomicActionOk_from_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'AtomicActionOk' from state "
				      "'Dismissed'"));
			}

			virtual void after_evAtomicActionOk_from_virt_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionOk' from state "
				      "'virt_Dismissed'"));
			}

			virtual void after_EqptDown_from_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'EqptDown' from state "
				      "'Dismissed'"));
			}

			virtual void after_evEqptDown_from_virt_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evEqptDown' from state "
				      "'virt_Dismissed'"));
			}

			virtual void after_evAtomicActionIncomplete_from_virt_Dismissed(void * context) {
				throw(std::string("Undefined transition after event "
				      "'evAtomicActionIncomplete' from state "
				      "'virt_Dismissed'"));
			}


		private:
			std::string name_;
		};

		/*
		 *  Classes that MUST be derived!!! - START
		 */
		class Closing_i;
		class Incomplete_i;
		class Creating_i;
		class Up_i;
		class Down_i;
		class Dismissed_i;

		class virt_Closing_i;
		class virt_Incomplete_i;
		class virt_Creating_i;
		class virt_Up_i;
		class virt_Down_i;
		class virt_Dismissed_i;



		class Closing : public State {
		public:
			Closing() :
				State(std::string("Closing")) { }
			virtual ~Closing() { }

			virtual fsm::base_TNRC::nextEvFor_AtomicActionOk_t AtomicActionOk(void* context) = 0;

			virtual fsm::base_TNRC::nextEvFor_AtomicActionDownTimeout_t AtomicActionDownTimeout(void* context) = 0;

			virtual void after_evAtomicActionDownTimeout_from_virt_Closing(void * context) = 0;

			virtual fsm::base_TNRC::nextEvFor_AtomicActionKo_t AtomicActionKo(void* context) = 0;

			virtual void after_evAtomicActionRetry_from_virt_Closing(void * context) = 0;

			virtual fsm::base_TNRC::nextEvFor_AtomicActionRetryTimer_t AtomicActionRetryTimer(void* context) = 0;

			virtual void after_evAtomicActionRetryTimer_from_virt_Closing(void * context) = 0;

			virtual void after_evAtomicActionNext_from_virt_Closing(void * context) = 0;

			virtual void after_evActionRollback_from_virt_Incomplete(void * context) = 0;

			virtual void after_evActionDestroy_from_virt_Up(void * context) = 0;

		};

		class Incomplete : public State {
		public:
			Incomplete() :
				State(std::string("Incomplete")) { }
			virtual ~Incomplete() { }

			virtual fsm::base_TNRC::nextEvFor_ActionRollback_t ActionRollback(void* context) = 0;

			virtual fsm::base_TNRC::nextEvFor_EqptDown_t EqptDown(void* context) = 0;

			virtual void after_evAtomicActionIncomplete_from_virt_Creating(void * context) = 0;

			virtual void after_evAtomicActionOk_from_virt_Dismissed(void * context) = 0;

			virtual void after_evAtomicActionIncomplete_from_virt_Dismissed(void * context) = 0;

		};

		class Creating : public State {
		public:
			Creating() :
				State(std::string("Creating")) { }
			virtual ~Creating() { }

			virtual fsm::base_TNRC::nextEvFor_AtomicActionKo_t AtomicActionKo(void* context) = 0;

			virtual fsm::base_TNRC::nextEvFor_ActionDestroy_t ActionDestroy(void* context) = 0;

			virtual fsm::base_TNRC::nextEvFor_EqptDown_t EqptDown(void* context) = 0;

			virtual fsm::base_TNRC::nextEvFor_AtomicActionOk_t AtomicActionOk(void* context) = 0;

			virtual void after_evAtomicActionNext_from_virt_Creating(void * context) = 0;

			virtual void after_evActionCreate_from_virt_Down(void * context) = 0;

		};

		class Up : public State {
		public:
			Up() :
				State(std::string("Up")) { }
			virtual ~Up() { }

			virtual void after_evActionEndUp_from_virt_Creating(void * context) = 0;

			virtual fsm::base_TNRC::nextEvFor_ActionNotification_t ActionNotification(void* context) = 0;

			virtual void after_evActionNotification_from_virt_Up(void * context) = 0;

			virtual fsm::base_TNRC::nextEvFor_ActionDestroy_t ActionDestroy(void* context) = 0;

		};

		class Down : public State {
		public:
			Down() :
				State(std::string("Down")) { }
			virtual ~Down() { }

			virtual void after_evActionEndDown_from_virt_Closing(void * context) = 0;

			virtual void after_evAtomicActionAbort_from_virt_Closing(void * context) = 0;

			virtual void after_evEqptDown_from_virt_Incomplete(void * context) = 0;

			virtual void after_evAtomicActionKo_from_virt_Creating(void * context) = 0;

			virtual void after_evEqptDown_from_virt_Creating(void * context) = 0;

			virtual void after_evActionDestroy_from_virt_Creating(void * context) = 0;

			virtual fsm::base_TNRC::nextEvFor_ActionCreate_t ActionCreate(void* context) = 0;

			virtual void after_evAtomicActionTimeout_from_virt_Dismissed(void * context) = 0;

			virtual void after_evAtomicActionKo_from_virt_Dismissed(void * context) = 0;

			virtual void after_evEqptDown_from_virt_Dismissed(void * context) = 0;

		};

		class Dismissed : public State {
		public:
			Dismissed() :
				State(std::string("Dismissed")) { }
			virtual ~Dismissed() { }

			virtual void after_evActionPending_from_virt_Creating(void * context) = 0;

			virtual fsm::base_TNRC::nextEvFor_AtomicActionTimeout_t AtomicActionTimeout(void* context) = 0;

			virtual fsm::base_TNRC::nextEvFor_AtomicActionKo_t AtomicActionKo(void* context) = 0;

			virtual fsm::base_TNRC::nextEvFor_AtomicActionOk_t AtomicActionOk(void* context) = 0;

			virtual fsm::base_TNRC::nextEvFor_EqptDown_t EqptDown(void* context) = 0;

		};

		class virt_Closing : public State {
		public:
			virt_Closing() :
				State(std::string("virt_Closing")) { }
			virtual ~virt_Closing() { }

			virtual void after_AtomicActionOk_from_Closing(void * context) = 0;

	                virtual bool evActionEndDown(void* context) = 0;

			virtual void after_AtomicActionDownTimeout_from_Closing(void * context) = 0;

	                virtual bool evAtomicActionDownTimeout(void* context) = 0;

			virtual void after_AtomicActionKo_from_Closing(void * context) = 0;

	                virtual bool evAtomicActionRetry(void* context) = 0;

	                virtual bool evAtomicActionAbort(void* context) = 0;

			virtual void after_AtomicActionRetryTimer_from_Closing(void * context) = 0;

	                virtual bool evAtomicActionRetryTimer(void* context) = 0;

	                virtual bool evAtomicActionNext(void* context) = 0;

		};

		class virt_Incomplete : public State {
		public:
			virt_Incomplete() :
				State(std::string("virt_Incomplete")) { }
			virtual ~virt_Incomplete() { }

			virtual void after_ActionRollback_from_Incomplete(void * context) = 0;

	                virtual bool evActionRollback(void* context) = 0;

			virtual void after_EqptDown_from_Incomplete(void * context) = 0;

	                virtual bool evEqptDown(void* context) = 0;

		};

		class virt_Creating : public State {
		public:
			virt_Creating() :
				State(std::string("virt_Creating")) { }
			virtual ~virt_Creating() { }

			virtual void after_AtomicActionKo_from_Creating(void * context) = 0;

	                virtual bool evAtomicActionKo(void* context) = 0;

			virtual void after_ActionDestroy_from_Creating(void * context) = 0;

	                virtual bool evActionPending(void* context) = 0;

			virtual void after_EqptDown_from_Creating(void * context) = 0;

	                virtual bool evEqptDown(void* context) = 0;

			virtual void after_AtomicActionOk_from_Creating(void * context) = 0;

	                virtual bool evActionEndUp(void* context) = 0;

	                virtual bool evAtomicActionNext(void* context) = 0;

	                virtual bool evActionDestroy(void* context) = 0;

	                virtual bool evAtomicActionIncomplete(void* context) = 0;

		};

		class virt_Up : public State {
		public:
			virt_Up() :
				State(std::string("virt_Up")) { }
			virtual ~virt_Up() { }

			virtual void after_ActionNotification_from_Up(void * context) = 0;

	                virtual bool evActionNotification(void* context) = 0;

			virtual void after_ActionDestroy_from_Up(void * context) = 0;

	                virtual bool evActionDestroy(void* context) = 0;

		};

		class virt_Down : public State {
		public:
			virt_Down() :
				State(std::string("virt_Down")) { }
			virtual ~virt_Down() { }

			virtual void after_ActionCreate_from_Down(void * context) = 0;

	                virtual bool evActionCreate(void* context) = 0;

		};

		class virt_Dismissed : public State {
		public:
			virt_Dismissed() :
				State(std::string("virt_Dismissed")) { }
			virtual ~virt_Dismissed() { }

			virtual void after_AtomicActionTimeout_from_Dismissed(void * context) = 0;

	                virtual bool evAtomicActionTimeout(void* context) = 0;

			virtual void after_AtomicActionKo_from_Dismissed(void * context) = 0;

	                virtual bool evAtomicActionKo(void* context) = 0;

			virtual void after_AtomicActionOk_from_Dismissed(void * context) = 0;

	                virtual bool evAtomicActionOk(void* context) = 0;

			virtual void after_EqptDown_from_Dismissed(void * context) = 0;

	                virtual bool evEqptDown(void* context) = 0;

	                virtual bool evAtomicActionIncomplete(void* context) = 0;

		};

		/*
		 *  Classes that MUST be derived!!! - END
		 */

		class BaseFSM {
		public:
			enum traceLevel_t {
				TRACE_DBG = 0,
				TRACE_LOG,
				TRACE_INF,
				TRACE_WRN,
				TRACE_ERR
			};

			BaseFSM(traceLevel_t level = TRACE_DBG) :
				level_(level),
				name_(std::string("TNRC")) { }
			virtual ~BaseFSM(void) { }

			std::string name(void) { return name_; }

			traceLevel_t traceLevel(void) {	return level_; }

			void traceLevel(traceLevel_t level) { level_ = level; }

			void dbg(std::string text) {
				DUMP(level_, TRACE_DBG, name_, text);
			}

			void log(std::string text) {
				DUMP(level_, TRACE_LOG, name_, text);
			}

			void inf(std::string text) {
				DUMP(level_, TRACE_INF, name_, text);
			}

			void wrn(std::string text) {
				DUMP(level_, TRACE_WRN, name_, text);
			}

			void err(std::string text) {
				DUMP(level_, TRACE_ERR, name_, text);
			}

		private:
			traceLevel_t level_;
		protected:
			std::string  name_;
		};

		/*
		 *  Class for checking FSM integrity ... not FULL IMPLEMENTED
		 */
		class GenericFSM : public BaseFSM {
		public:
			GenericFSM(traceLevel_t level = TRACE_DBG) :
				BaseFSM(level),
				changeInProgress_(false) { }
			virtual ~GenericFSM(void) { }

			bool startModify(void);
			bool endModify(void);

			//TODO: callback????
			typedef void (* callback_t) (std::string from_state,
						     std::string to_state,
						     std::string on_event,
						     void *      context);

			//// CallBack
			//bool addPreCallBack(callback_t cb,
			//		   void *     context);
			//bool addPostCallback(callback_t cb,
			//		    void *     context);
			//bool addInCallback(callback_t cb,
			//		  void * context);
			//
			//bool remPreCallBack(callback_t cb);
			//bool remPostCallback(callback_t cb);
			//bool remInCallback(callback_t cb);


			// States
			bool addState(std::string state);
			bool remState(std::string state);

			// Events
			bool addEvent(std::string event);
			bool remEvent(std::string event);

			// Transitions
			bool addTransition(std::string from,
					   std::string to,
					   std::string event);
			bool remTransition(std::string from,
					   std::string to,
					   std::string event);

			// General
			bool setStartState(std::string state);

			//bool run(std::string event);
			//std::string getState(void);

			//operator<<
			//operator>>

		private:
			bool check(void);

			struct state_data_t {
				callback_t  pre;
				callback_t  post;
				callback_t  in;
			};

			Matrix<std::string,
			       std::string,
			       std::string>                 transitions_;
			std::map<callback_t, void *>        contexts_;
			std::map<std::string, state_data_t> states_;
			std::list<std::string>              events_;
			bool                                changeInProgress_;
			std::string                         startState_;
		};

		class Fsm : public GenericFSM {
		public:
			/*  add/rem of states/events/callback for
			 *  checking FSM consistency
			 */
			Fsm(traceLevel_t level = TRACE_DBG)
				throw(std::string);

			virtual ~Fsm(void);

			friend std::ostream& operator<<(std::ostream&  s,
							const Fsm& f);


			bool evAtomicActionRetryTimer(void * context);

			bool evActionDestroy(void * context);

			bool evActionPending(void * context);

			bool evAtomicActionDownTimeout(void * context);

			bool evActionNotification(void * context);

			bool evAtomicActionOk(void * context);

			bool evAtomicActionNext(void * context);

			bool evActionEndUp(void * context);

			bool evActionEndDown(void * context);

			bool evAtomicActionTimeout(void * context);

			bool evEqptDown(void * context);

			bool evActionRollback(void * context);

			bool evAtomicActionKo(void * context);

			bool evAtomicActionRetry(void * context);

			bool evAtomicActionIncomplete(void * context);

			bool evAtomicActionAbort(void * context);

			bool evActionCreate(void * context);

			nextEvFor_AtomicActionRetryTimer_t AtomicActionRetryTimer(void * context);

			nextEvFor_ActionDestroy_t ActionDestroy(void * context);

			nextEvFor_AtomicActionDownTimeout_t AtomicActionDownTimeout(void * context);

			nextEvFor_ActionNotification_t ActionNotification(void * context);

			nextEvFor_AtomicActionOk_t AtomicActionOk(void * context);

			nextEvFor_AtomicActionTimeout_t AtomicActionTimeout(void * context);

			nextEvFor_EqptDown_t EqptDown(void * context);

			nextEvFor_ActionRollback_t ActionRollback(void * context);

			nextEvFor_AtomicActionKo_t AtomicActionKo(void * context);

			nextEvFor_ActionCreate_t ActionCreate(void * context);


			//operator<<
			//operator>>

			State * currentState(void);
			bool    go2prevState(void);

		private:
			enum states_t {
				TNRC_Closing,
				TNRC_Incomplete,
				TNRC_Creating,
				TNRC_Up,
				TNRC_Down,
				TNRC_Dismissed,
				TNRC_virt_Closing,
				TNRC_virt_Incomplete,
				TNRC_virt_Creating,
				TNRC_virt_Up,
				TNRC_virt_Down,
				TNRC_virt_Dismissed,

			};

			enum events_t {
				TNRC_evAtomicActionRetryTimer,
				TNRC_evActionDestroy,
				TNRC_evActionPending,
				TNRC_evAtomicActionDownTimeout,
				TNRC_evActionNotification,
				TNRC_evAtomicActionOk,
				TNRC_evAtomicActionNext,
				TNRC_evActionEndUp,
				TNRC_evActionEndDown,
				TNRC_evAtomicActionTimeout,
				TNRC_evEqptDown,
				TNRC_evActionRollback,
				TNRC_evAtomicActionKo,
				TNRC_evAtomicActionRetry,
				TNRC_evAtomicActionIncomplete,
				TNRC_evAtomicActionAbort,
				TNRC_evActionCreate,
				TNRC_AtomicActionRetryTimer,
				TNRC_ActionDestroy,
				TNRC_AtomicActionDownTimeout,
				TNRC_ActionNotification,
				TNRC_AtomicActionOk,
				TNRC_AtomicActionTimeout,
				TNRC_EqptDown,
				TNRC_ActionRollback,
				TNRC_AtomicActionKo,
				TNRC_ActionCreate,

			};

			friend std::ostream& operator<<(std::ostream&   s,
							const states_t& st);

			friend std::ostream& operator<<(std::ostream&   s,
							const events_t& ev);

			states_t                             currentState_;
			states_t                             prevState_;
			std::map<states_t, State *>          states_;
			Matrix<states_t, events_t, states_t> nextState_;
		};
        }
#endif // NAMESPACE_TNRC




	/*************************************/
	/*   Finite State Machine - Wrapper  */
	/*************************************/



#ifndef NAMESPACE_TNRC
#define NAMESPACE_TNRC

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <map>
#include <list>
#include <iostream>

	namespace TNRC {

		class virtFsm {
		public:
			virtFsm(base_TNRC::BaseFSM::traceLevel_t
				level =	base_TNRC::BaseFSM::TRACE_DBG)
				throw(std::string);

			virtual ~virtFsm(void);

			friend std::ostream& operator<<(std::ostream&  s,
							const virtFsm& f);

			enum root_events_t {
                                //TNRC_invalid_event,
				TNRC_AtomicActionRetryTimer,
				TNRC_ActionDestroy,
				TNRC_AtomicActionDownTimeout,
				TNRC_ActionNotification,
				TNRC_AtomicActionOk,
				TNRC_AtomicActionTimeout,
				TNRC_EqptDown,
				TNRC_ActionRollback,
				TNRC_AtomicActionKo,
				TNRC_ActionCreate,

			};

			void post(root_events_t ev, void * context, bool enqueue = false);

			std::string currentState(void);

		private:
			void runPendingWork(void);


			void AtomicActionRetryTimer(void * context);

			void ActionDestroy(void * context);

			void AtomicActionDownTimeout(void * context);

			void ActionNotification(void * context);

			void AtomicActionOk(void * context);

			void AtomicActionTimeout(void * context);

			void EqptDown(void * context);

			void ActionRollback(void * context);

			void AtomicActionKo(void * context);

			void ActionCreate(void * context);


			typedef struct {
				root_events_t ev;
				void *        context;
			} data_event_t;

			base_TNRC::Fsm * fsm_;
			std::list<data_event_t *>  events_;
		};

	}
#endif // NAMESPACE_TNRC

}

#endif // FSMGEN_H
