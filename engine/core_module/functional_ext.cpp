/*
   FALCON - The Falcon Programming Language.
   FILE: functional_ext.cpp

   Functional programming support
   -------------------------------------------------------------------
   Author: Giancarlo Niccolai
   Begin: Thu, 14 Aug 2008 02:10:57 +0200

   -------------------------------------------------------------------
   (C) Copyright 2008: the FALCON developers (see list in AUTHORS file)

   See LICENSE file for licensing details.
*/

#include "core_module.h"

/*#

*/
namespace Falcon {
namespace core {

/*#
   @funset functional_support Functional programming support
   @brief ETA functions and functional constructs.

   Falcon provides some special functional programming constructs that are known
   to the VM to have special significance. The vast majority of them starts a
   “functional evaluation” chain on their parameters before their value is evaluated.
   A functional evaluation is a recursive evaluation (reduction) of list structures into
   atoms. At the moment, the only list structure that can be evaluated this way is the array.
   Evaluating a parameter in functional context means that the given parameter will be
   recursively scanned for callable arrays or symbols that can be reduced to atoms. A callable
   array is reduced by calling the function and substituting it with its return value.
   When all the contents of the list are reduced, the higher level is evaluated.

   Consider this example:
   @code
   function func0( p0, p1 ): ...
   function func1( p0 ): ...

   list = [func0, [func1, param1], param2]
   @endcode

   Calling @b list as a callable array, func0 will be called with the array [func1, param1] as
   the first parameter, and param2 as the second parameter. On the other hand, evaluating
   the above list in a functional context, first func1 will be called with param1, then
   func0 will be called with the return value of the previous evaluation as the first parameter,
   and with param2 as the second parameter.

   The functions in this section are considered “special constructs” as the VM knows them and
   treats them specially. Their definition overrides the definition of a functional evaluation,
   so that when the VM finds a special construct in its evaluation process, it ceases using the
   default evaluation algorithm and passes evaluation control to the construct.

   In example, the iff construct selects one of its branches to be evaluated only if the first
   parameter evaluates to true:
   @code
   list = [iff, someValueIsTrue, [func0, [func1, param1]], [func1, param2] ]
   @endcode

   If this list had to be evaluated in a functional context, then before iff had a chance to
   decide what to do, the two arrays [func0, ...] and [func1,...] would have been evaluated.
   As iff is a special construct, the VM doesn't evaluate its parameters and lets iff perform
   its operations as it prefer. In the case o iff, it first evaluates the first parameter,
   then evaluates in functional context the second on the third parameter,
   leaving unevaluated the other one.

   Not all constructs evaluates everything it is passed to them in a functional context. Some of
   them are meant exactly to treat even a callable array (or anything else that should be reduced)
   as-is, stopping the evaluation process as the VM meets them. The description of each construct
   explains its working principles, and whether if its parameters are  evaluated or not.

   Please, notice that “callable” doesn't necessarily mean “evaluable”. To evaluate in functional
   context a callable symbol without parameter, it must be transformed into a single-element array.
   In example:
   @code
   function func0(): ...

   result = [iff, shouldEval, [func0], func0]
   @endcode

   This places in result the value returned by func0 if shouldEval is true, while it returns exactly
   the function object func0 as-is if shouldEval is false.

   A more formal definition of the funcional programming support  in Falcon is provided in the
   Survival Guide.
*/

static bool internal_eq( ::Falcon::VMachine *vm, const Item &first, const Item &second )
{
   if( first == second || vm->compareItems( first, second ) == 0 )
   {
      return true;
   }

   if( first.isArray() && second.isArray() )
   {
      CoreArray *arr1 = first.asArray();
      CoreArray *arr2 = second.asArray();

      if ( arr1->length() != arr2->length() )
         return false;

      for ( uint32 p = 0; p < arr1->length(); p++ )
      {
         if ( ! internal_eq( vm, arr1->at(p), arr2->at(p) ) )
            return false;
      }

      return true;
   }

   if( first.isDict() && second.isDict() )
   {
      CoreDict *d1 = first.asDict();
      CoreDict *d2 = second.asDict();

      if ( d1->length() != d2->length() )
         return false;

      DictIterator *di1 = d1->first();
      DictIterator *di2 = d2->first();
      while( di1->isValid() )
      {
         if ( ! internal_eq( vm, di1->getCurrentKey(), di2->getCurrentKey() ) ||
              ! internal_eq( vm, di1->getCurrent(), di2->getCurrent() ) )
         {
            delete d1;
            delete d2;
            return false;
         }
      }

      delete d1;
      delete d2;
      return true;
   }

   return false;
}


FALCON_FUNC  core_eq( ::Falcon::VMachine *vm )
{
   Item *first = vm->param(0);
   Item *second = vm->param(1);
   if ( first == 0 || second == 0 )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params).extra( "X,X" ) ) );
      return;
   }

   vm->retval( internal_eq( vm, *first, *second ) ? 1:0);
}

static bool core_any_next( ::Falcon::VMachine *vm )
{
   // was the elaboration succesful?
   if ( vm->regA().isTrue() )
   {
      vm->retval( (int64) 1 );
      return false;
   }

   // repeat checks.
   CoreArray *arr = vm->param(0)->asArray();
   uint32 count = (uint32) vm->local(0)->asInteger();
   while( count < arr->length() )
   {
      Item *itm = &arr->at(count);
      *vm->local(0) = (int64) count+1;
      if ( vm->functionalEval( *itm  ) )
      {
         return true;
      }
      else if ( vm->regA().isTrue() ) {
         vm->retval( (int64) 1 );
         return false;
      }
      count++;
   }

   vm->retval( (int64) 0 );
   return false;
}


/*#
   @function any
   @inset functional_support
   @brief Returns true if any of the items in a given collection evaluate to true.
   @param sequence A sequence of arbitrary items.
   @return true at least one item in the collection is true, false otherwise.

   Items in @b sequence are evaluated in functional context for truth value. This means that,
   if they are sigmas, they get sigma-reduced and their return value is evaluated,
   otheriwise they are evaluated directly.

   Truth value is determined using the standard Falcon truth
   check (nil is false, numerics are true if not zero, strings and collections are true if not
   empty, object and classes are always true).

   The check is short circuited. This means that elements are evaluated until
   an element considered to be true (or sigma-reduced to a true value) is found.

   If the collection is empty, this function returns false.
*/
FALCON_FUNC  core_any ( ::Falcon::VMachine *vm )
{
   Item *i_param = vm->param(0);
   if( i_param == 0 || !i_param->isArray() )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "A" ) ) );
      return;
   }

   CoreArray *arr = i_param->asArray();
   uint32 count = arr->length();
   vm->returnHandler( core_any_next );
   vm->addLocals(1);

   for( uint32 i = 0; i < count; i ++ )
   {
      Item *itm = &arr->at(i);
      *vm->local(0) = (int64) i+1;
      if ( vm->functionalEval( *itm  ) )
      {
         return;
      }
      else if ( vm->regA().isTrue() ) {
         vm->returnHandler( 0 );
         vm->retval( (int64) 1 );
         return;
      }
   }

   vm->returnHandler( 0 );
   vm->retval( (int64) 0 );
}


static bool core_all_next( ::Falcon::VMachine *vm )
{
   // was the elaboration succesful?
   if ( ! vm->regA().isTrue() )
   {
      vm->retval( (int64) 0 );
      return false;
   }

   // repeat checks.
   CoreArray *arr = vm->param(0)->asArray();
   uint32 count = (uint32) vm->local(0)->asInteger();
   while( count < arr->length() )
   {
      Item *itm = &arr->at(count);

      *vm->local(0) = (int64) count+1;
      if ( vm->functionalEval( *itm  ) )
      {
         return true;
      }
      else if ( ! vm->regA().isTrue() ) {
         vm->retval( (int64) 0 );
         return false;
      }
      count++;
   }

   vm->retval( (int64) 1 );
   return false;
}

/*#
   @function all
   @inset functional_support
   @brief Returns true if all the items in a given collection evaluate to true.
   @param sequence A sequence of arbitrary items.
   @return true if all the items are true, false otherwise

   Items in @b sequence are evaluated in functional context for truth value. This means that,
   if they are sigmas, they get sigma-reduced and their return value is evaluated,
   otheriwise they are evaluated directly.

   Truth value is determined using the standard Falcon truth
   check (nil is false, numerics are true if not zero, strings and collections are true if not
   empty, object and classes are always true).

   The check is short circuited. This means that the processing of parameters
   is interrupted as an element is evaluated into false.

   If the collection is empty, this function returns false.
*/

FALCON_FUNC  core_all ( ::Falcon::VMachine *vm )
{
   Item *i_param = vm->param(0);
   if( i_param == 0 || !i_param->isArray() )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "A" ) ) );
      return;
   }

   CoreArray *arr = i_param->asArray();
   uint32 count = arr->length();
   if ( count == 0 )
   {
      vm->retval( (int64) 0 );
      return;
   }

   vm->returnHandler( core_all_next );
   vm->addLocals(1);

   for( uint32 i = 0; i < count; i ++ )
   {
      Item *itm = &arr->at(i);
      *vm->local(0) = (int64) i+1;

      if ( vm->functionalEval( *itm  ) )
      {
         return;
      }
      else if ( ! vm->regA().isTrue() ) {
         vm->returnHandler( 0 );
         vm->retval( (int64) 0 );
         return;
      }
   }

   vm->returnHandler( 0 );
   vm->retval( (int64) 1 );
}


static bool core_anyp_next( ::Falcon::VMachine *vm )
{
   // was the elaboration succesful?
   if ( vm->regA().isTrue() )
   {
      vm->retval( (int64) 1 );
      return false;
   }

   // repeat checks.
   int32 count = (uint32) vm->local(0)->asInteger();
   while( count < vm->paramCount() )
   {
      Item *itm = vm->param( count );
      *vm->local(0) = (int64) count+1;

      if ( vm->functionalEval( *itm  ) )
      {
         return true;
      }
      else if ( vm->regA().isTrue() ) {
         vm->retval( (int64) 1 );
         return false;
      }
      count++;
   }

   vm->retval( (int64) 0 );
   return false;
}

/*#
   @function anyp
   @inset functional_support
   @brief Returns true if any one of the parameters evaluate to true.
   @param ... A list of arbitrary items.
   @return true at least one parameter is true, false otherwise.

   This function works like @a any, but the sequence may be specified directly
   in the parameters rather being given in a separate array. This make easier to write
   anyp in callable arrays. In example, one may write
   @code
      [anyp, 1, k, n ...]
   @endcode
   while using any one should write
   @code
      [any, [1, k, n ...]]
   @endcode

   Parameters are evaluated in functional context. This means that,
   if they are sigmas, they get sigma-reduced and their return value is evaluated,
   otheriwise they are evaluated directly.

   Truth value is determined using the standard Falcon truth
   check (nil is false, numerics are true if not zero, strings and collections are true if not
   empty, object and classes are always true).

   If called without parameters, this function returns false.
*/
FALCON_FUNC  core_anyp ( ::Falcon::VMachine *vm )
{
   uint32 count = vm->paramCount();
   vm->returnHandler( core_anyp_next );
   vm->addLocals(1);

   for( uint32 i = 0; i < count; i ++ )
   {
      Item *itm = vm->param(i);
      *vm->local(0) = (int64) i+1;

      if ( vm->functionalEval( *itm  ) )
      {
         return;
      }
      else if ( vm->regA().isTrue() ) {
         vm->returnHandler( 0 );
         vm->retval( (int64) 1 );
         return;
      }
   }

   vm->returnHandler( 0 );
   vm->retval( (int64) 0 );
}


static bool core_allp_next( ::Falcon::VMachine *vm )
{
   // was the elaboration succesful?
   if ( ! vm->regA().isTrue() )
   {
      vm->retval( (int64) 0 );
      return false;
   }

   // repeat checks.
   int32 count = (uint32) vm->local(0)->asInteger();
   while( count < vm->paramCount() )
   {
      Item *itm = vm->param(count);

      *vm->local(0) = (int64) count+1;
      if ( vm->functionalEval( *itm  ) )
      {
         return true;
      }
      else if ( ! vm->regA().isTrue() ) {
         vm->retval( (int64) 0 );
         return false;
      }
      count++;
   }

   vm->retval( 1 );
   return false;
}

/*#
   @function allp
   @inset functional_support
   @brief Returns true if all the parameters evaluate to true.
   @param ... An arbitrary list of items.
   @return true if all the items are true, false otherwise

   This function works like @a all, but the collection may be specified directly
   in the parameters rather being given in a separate array. This make easier to
   write allp in callable arrays. In example, one may write
   @code
      [allp, 1, k, n ...]
   @endcode
   while using all one should write
   @code
      [all, [1, k, n ...]]
   @endcode

   Parameters are evaluated in functional context. This means that,
   if they are sigmas, they get sigma-reduced and their return value is evaluated,
   otheriwise they are evaluated directly.

   Truth value is determined using the standard Falcon truth
   check (nil is false, numerics are true if not zero, strings and collections are true if not
   empty, object and classes are always true).

   If called without parameters, this function returns false.
*/
FALCON_FUNC  core_allp ( ::Falcon::VMachine *vm )
{
   uint32 count = vm->paramCount();
   vm->returnHandler( core_allp_next );
   vm->addLocals(1);

   if ( count == 0 )
   {
      vm->retval(0);
      return;
   }

   for( uint32 i = 0; i < count; i ++ )
   {
      Item *itm = vm->param(i);
      *vm->local(0) = (int64) i+1;
      if ( vm->functionalEval( *itm  ) )
      {
         return;
      }
      else if ( ! vm->regA().isTrue() ) {
         vm->returnHandler( 0 );
         vm->retval( (int64) 0 );
         return;
      }
   }

   vm->returnHandler( 0 );
   vm->retval( 1 );
}


/*#
   @function eval
   @inset functional_support
   @brief Evaluates a sequence in functional context.
   @param sequence A sequence to be evaluated.
   @return The sigma-reduction (evaluation) result.

   The parameter is evaluated in functional context; this means that if the parameter
   is a sequence starting with a callable item, that item gets called with the rest of the
   sequence passed as parameters, and the result it returns is considered the
   "evaluation result". This is performed recursively, inner-to-outer, on every element
   of the sequence before the call to the first element is actually performed.

   The description of the functional evaluation algorithm is included in the heading
   of this section.
*/

FALCON_FUNC  core_eval ( ::Falcon::VMachine *vm )
{
   Item *i_param = vm->param(0);
   if( i_param == 0 )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "X" ) ) );
      return;
   }

   vm->functionalEval( *i_param );
}


FALCON_FUNC  core_min ( ::Falcon::VMachine *vm )
{
   if ( vm->paramCount() == 0 )
   {
      vm->retnil();
      return;
   }

   Item *elem = vm->param( 0 );
   for ( int32 i = 1; i < vm->paramCount(); i++)
   {
      if ( vm->compareItems( *vm->param(i), *elem ) < 0 )
      {
         elem = vm->param(i);
      }

      if (vm->hadEvent())
         return;
   }

   vm->retval( *elem );
}

FALCON_FUNC  core_max ( ::Falcon::VMachine *vm )
{
   if ( vm->paramCount() == 0 )
   {
      vm->retnil();
      return;
   }

   Item *elem = vm->param( 0 );
   int32 count = vm->paramCount();
   for ( int32 i = 1; i < count; i++)
   {
      if ( vm->compareItems( *vm->param(i), *elem ) > 0 )
      {
         elem = vm->param(i);
      }

      if (vm->hadEvent())
         return;
   }

   vm->retval( *elem );
}

/*#
   @function map
   @inset functional_support
   @brief Creates a new vector of items transforming each item in the original array through the mapping function.
   @param mfunc A function or sigma used to map the array.
   @param sequence A sequence of arbitrary items.
   @return The parameter unevaluated.

   mfunc is called iteratively for every item in the collection; its return value is added to the
   mapped array. In this way it is possible to apply an uniform transformation to all the item
   in a collection.

   If mfunc returns an out of band nil item, map skips the given position in the target array,
   actually acting also as a filter function.

   In example:
   @code
      function mapper( item )
         if item < 0: return oob(nil)  // discard negative items
         return item ** 0.5            // perform square root
      end

   inspect( map( mapper, [ 100, 4, -12, 9 ]) )    // returns [10, 2, 3]
   @endcode

   @see oob
*/

static bool core_map_next( ::Falcon::VMachine *vm )
{
   // callable in first item
   CoreArray *origin = vm->param(1)->asArray();
   uint32 count = (uint32) vm->local(0)->asInteger();
   CoreArray *mapped = vm->local(1)->asArray();

   if ( ! vm->regA().isOob() )
      mapped->append( vm->regA() );

   if ( count < origin->length() )
   {
      *vm->local(0) = (int64) count + 1;
      vm->pushParameter( origin->at(count) );
      vm->callFrame( *vm->param(0), 1 );
      return true;
   }

   vm->retval( mapped );
   return false;
}

FALCON_FUNC  core_map ( ::Falcon::VMachine *vm )
{
   Item *callable = vm->param(0);
   Item *i_origin = vm->param(1);
   if( callable == 0 || !callable->isCallable() ||
       i_origin == 0 || !i_origin->isArray()
      )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "C,A" ) ) );
      return;
   }

   CoreArray *origin = i_origin->asArray();
   CoreArray *mapped = new CoreArray( vm, origin->length() );
   if ( origin->length() > 0 )
   {
      vm->returnHandler( core_map_next );
      vm->addLocals( 2 );
      *vm->local(0) = (int64)1;
      *vm->local(1) = mapped;

      vm->pushParameter( origin->at(0) );
      // do not use pre-fetched pointer
      vm->callFrame( *vm->param(0), 1 );
      return;
   }

   vm->retval( mapped );
}

static bool core_dolist_next ( ::Falcon::VMachine *vm )
{
   CoreArray *origin = vm->param(1)->asArray();
   uint32 count = (uint32) vm->local(0)->asInteger();

   // done -- let A stay as is.
   if ( count >= origin->length() )
      return false;

   //if we called
   if ( vm->local(1)->asInteger() == 1 )
   {
      // not true? -- exit
      if ( ! vm->regA().isTrue() )
      {
         return false;
      }

      // prepare for next loop
      *vm->local(1) = (int64)0;
      if ( vm->functionalEval( origin->at(count) ) )
      {
         return true;
      }
   }

   *vm->local(0) = (int64) count + 1;
   *vm->local(1) = (int64) 1;
   vm->pushParameter( vm->regA() );
   vm->callFrame( *vm->param(0), 1 );
   return true;
}

/*#
   @function dolist
   @inset functional_support
   @brief Repeats an operation on a list of parameters.
   @param processor A callable item that will receive data coming from the sequence.
   @param sequence A list of items that will be fed in the processor one at a time.
   @optparam ... Optional parameters to be passed to the first callable item.
   @return The return value of the last callable item.

   Every item in @b sequence is passed as parameter to the processor, which must be a callable
   item. Items are also functionally evaluated, one by one, but the parameter @b sequence is not
   functionally evaluated as a whole; to do that, use the explicit evaluation:
   @code
      dolist( processor, eval(array) )
   @endcode
   This method is equivalent to @a xmap, but it has the advantage that it doesn't create an array
   of evaluated results. So, when it is not necessary to transform a sequence in another through a
   mapping function, but just to run repeatedly over a collection, this function is to be preferred.
*/
FALCON_FUNC  core_dolist ( ::Falcon::VMachine *vm )
{
   Item *callable = vm->param(0);
   Item *i_origin = vm->param(1);
   if( callable == 0 || !callable->isCallable() ||
       i_origin == 0 || !i_origin->isArray()
      )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "C,A" ) ) );
      return;
   }

   CoreArray *origin = i_origin->asArray();
   if ( origin->length() != 0 )
   {
      vm->returnHandler( core_dolist_next );
      vm->addLocals( 2 );
      // count
      *vm->local(0) = (int64) 0;

      //exiting from an eval or from a call frame? -- 0 eval
      *vm->local(1) = (int64) 0;

      if ( vm->functionalEval( origin->at(0) ) )
      {
         return;
      }

      // count
      *vm->local(0) = (int64) 1;

      //exiting from an eval or from a call frame? -- 1 callframe
      *vm->local(1) = (int64) 1;
      vm->pushParameter( vm->regA() );
      vm->callFrame( *vm->param(0), 1 );
   }
}


static bool core_times_next ( ::Falcon::VMachine *vm )
{
   // we may mangle with the parameters -- be careful.
   Item var = *vm->param(1);
   CoreArray *sequence = vm->param(2)->asArray();
   Item &range = *vm->local(0); // temporary reference, we won't hold for long.
   int32 start = range.asRangeStart();
   uint32 currentItemID = (uint32) vm->local(1)->asInteger();

   // Continue or items terminated?
   if( currentItemID == sequence->length() ||
      ( vm->regA().isOob() && vm->regA().isInteger() && vm->regA().asInteger() == 1 )
      )
   {
      currentItemID = 0;
      // here it is safe to change the reference
      start += range.asRangeStep();
      if ( vm->isParamByRef( 1 ) )
         vm->param(1)->setInteger( start );

      // we won't need the range past this point.
      range.setRange( start, range.asRangeEnd(), range.asRangeStep(), false );
   }

   // Break or loop terminated?
   if( ( range.asRangeStep() > 0 && start >= range.asRangeEnd()) ||
       ( range.asRangeStep() < 0 && start < range.asRangeEnd()) ||
      ( vm->regA().isOob() && vm->regA().isInteger() && vm->regA().asInteger() == 0 )
      )
   {
      vm->regA().setInteger( (int64) range.asRangeStart() );
      return false;
   }

   // get the current item.
   Item &current = sequence->at( currentItemID );

   // prepare the next current item ID
   vm->local(1)->setInteger( currentItemID + 1 );

   // Is the current item callable? -- if not, raise an error
   if ( ! current.isCallable() )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "uncallable" ) ) );
      return false;   // don't call me anymore
   }

   // how should we call the item?
   if ( ! vm->isParamByRef( 1 ) )
   {
      // we must alter its parameters.
      if ( current.isArray() )
      {
         CoreArray *curArray = current.asArray();
         // append? -- explode the call (callFrame would do it anyhow)
         int64 varID = var.forceInteger();
         if ( varID <= 0 )
         {
            for ( uint32 i = 1; i < curArray->length(); i++ )
            {
               vm->pushParameter( curArray->at(i) );
            }
            vm->pushParameter( (int64) start);
            // queue the call ( + 1 parameter - 1 because first item is callable)
            vm->callFrame( curArray->at(0), curArray->length() );
         }
         // otherwise, mangle the item ID
         else {
            // for now, let's ignore too short arrays.
            if ( curArray->length() > (uint32) varID )
            {
               curArray->at( (uint32) varID ) = (int64) start;
            }
            // just perform the call as-is.
            vm->callFrame( curArray, 0 );
         }
      }
      // if it's not an array, just push the parameter and call
      else {
         vm->pushParameter( (int64) start);
         vm->callFrame( current, 1 );
      }
   }
   else {
      // we need just to call the item.
      vm->callFrame( current, 0 );
   }

   // prepare the next current item ID
   vm->local(1)->setInteger( currentItemID + 1 );

   return true;
}

/*#
   @function times
   @inset functional_support
   @brief Repeats a sequence a determined number of times.
   @param count Count of times to be repeated or non-open range.
   @param var A reference to a variable that will receive the current count, nil or a number.
   @param sequence A list of callable items that can be called one at a time.
   @return Last index processed.

   This function is very similar to a functional for/in loop. It repeats a sequence
   of callable items in the @b sequence parameter a determined number of
   times, eventually filling a variable with the current loop index, or mangling the
   parameters of the given callable items so that they receive the index as a parameter.

   @note The paramters of @b times are not functionally evaluated.

   The loop index count will be given values from 0 to the required index-1 if @b count is numeric,
   or it will act as the for/in loop if @b count is a range.

   The way the current index loop is sent to the items depends on the type of @b var.
   If it's nil, then the count is only kept internally; Sigma functions in @b sequence may not need it, or
   they may use an internal counter. In example:
   @code
      function printTimes()
         static: i = 0
         > "Called ", ++i, " times."
      end

      times( 10, nil, [ printTimes ] )
   @endcode

   If @b val is a reference to a variable, then that variable is
   updated to the current loop value. The Sigmas in @b sequence may receive it as a parameter
   passed by reference or may accesses it from the outer (global) scope. In example:
   @code
      // module scope
      sent = nil

      function printSent()
         global sent
         > "Called ", sent, " times."
      end

      function printParam( var )
         > "Parameter is... ", var
      end

      times( 10, $sent, [ printSent, [printParam, $sent] ] )
   @endcode

   In the above example, printSent "fishes" the global value of @b sent, while printParam
   uses a reference to it in its parameters and sees its paramter list changed at each call.

   Finally, @b var may be a number. If the number is zero or less, the loop variable is just
   appended to the parameters in the call. The following example prints a list of pair numbers
   between 2 and 10:

   @code
      times( [2:11:2],     // range 2 to 10+1, with step 2
         0,                // instruct times to add the loop index to the calls
         .[ .[ printl "Index is now..." ] ]      // the calls (just 1).
         )
   @endcode

   If it's a positive number, then the nth element of the Sigmas in the list will be
   changed. In this last case, the items in @b sequence need not just to be callable; they
   must be Sigmas (lists starting with a callable item) having at least enough items for
   the @b var ID to be meaningful. The next example alters the parameter element #2 in the
   Sigmas array it calls:

   @code
      times( [2:11:2], 2,
         .[ .[ printl "Index is now... "
                 nil
                 " ..." ] ]
         )
   @endcode

   Notice the "nil" at position 2 in the Sigma call of printl. It may actually be any item, as it will be
   changed each time before the sigma is called.

   In this case, if the callable items in @b sequence are not sigmas, or if they are to short for the
   @b var ID to be useful, they get called without the addition of the loop index parameter.

   @note The original sigmas are not restored after @b times is executed in this modality. This means that the
   arrays in @b sequence will be altered, and they will hold the last number set by @b times before exit.

   Exactly like @a floop, the flow of calls in @b times can be altered by the functions in sequence returning
   an out-of-band 0 or 1. If any function in the sequence returns an out-of-band 0, @b times terminates and
   return immediately (performing an operation similar to "break"). If a function returns an out-of-band 1,
   the rest of the items in @b sequence are ignored, and the loop is restarted with the index updated; this
   is equivalent to a functional "continue". In example:

   @code
   times( 10, 0,
      .[ (function(x); if x < 5: return oob(1); end)   // skip numbers less than 5
         printl // print the others
       ]
    )
   @endcode

   The @b times function return the last generated value for the index. A natural termination of @b times
   can be detected thanks to the fact that the index is equal to the upper bound of the range, while
   an anticipated termination causes @b times to return a different index. In example, if @b count is
   10, the generated index (possibly received by the items in @b sequence) will range from 0 to 9 included,
   and if the function terminates correctly, it will return 10. If a function in @b sequence returns an
   out-of-band 0, causing a premature termination of the loop, the value returned by times will be the loop
   index at which the out-of-band 0 was returned.

   @note Ranges [m:n] where m > n (down-ranges) terminate at n included; in that case, a succesful
   completion of @b times return one-past n.
*/
FALCON_FUNC  core_times ( ::Falcon::VMachine *vm )
{
   Item *i_count = vm->param(0);
   Item *i_var = vm->param(1);
   Item *i_sequence = vm->param(2);

   if( i_count == 0 || ! ( i_count->isRange() || i_count->isOrdinal() ) ||
       i_var == 0 || ! ( vm->isParamByRef( 1 ) || i_var->isNil() || i_var->isOrdinal() ) ||
       i_sequence == 0 || ! i_sequence->isArray()
      )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "N|R, $|Nil|N, A" ) ) );
      return;
   }

   CoreArray *origin = i_sequence->asArray();
   int32 start, end, step;
   if( i_count->isRange() )
   {
      if ( i_count->asRangeIsOpen() )
      {
         vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
            extra( "open range" ) ) );
         return;
      }

      start = i_count->asRangeStart();
      end = i_count->asRangeEnd();
      step = i_count->asRangeStep();
      if ( step == 0 ) step = start > end ? -1 : 1;
   }
   else {
      start = 0;
      end = (int32) i_count->forceInteger();
      step = end < 0 ? -1 : 1;
   }

   CoreArray *sequence = i_sequence->asArray();

   // check ranges and steps.
   if ( start == end ||
        ( start < end && ( step < 0 || start + step > end ) ) ||
        ( start > end && ( step > 0 || start + step < end ) ) ||
        sequence->length() == 0
    )
   {
      // no loop to be done.
      vm->retval( (int64) start );
      return;
   }

   // ok, we must do at least a loop
   vm->returnHandler( core_times_next );

   // 1: shifting range
   // 2: position in the sequence calls.
   vm->addLocals( 2 );
   // count
   vm->local(0)->setRange( start, end, step, false);
   *vm->local(1) = (int64) 0;

   // prevent dirty A to mess our break/continue system.
   vm->regA().setNil();

   // eventually, set the initial count
   if ( vm->isParamByRef( 1 ) )
   {
      *i_var = (int64) start;
   }

   // ready; now the VM will call core_times_next
}

static bool core_xmap_next( ::Falcon::VMachine *vm )
{
   // in vm->param(0) there is "callable".
   CoreArray *origin = vm->param(1)->asArray();
   uint32 count = (uint32) vm->local(0)->asInteger();
   CoreArray *mapped = vm->local(1)->asArray();


   if ( count < origin->length() )
   {
      if ( vm->local(2)->asInteger() == 1 )
      {
         if ( ! vm->regA().isOob() )
            mapped->append( vm->regA() );

         // prepare for next loop
         *vm->local(0) = (int64) count + 1;
         *vm->local(2) = (int64) 0;
         if ( vm->functionalEval( origin->at(count) ) )
         {
            return true;
         }
      }

      *vm->local(2) = (int64) 1;
      vm->pushParameter( vm->regA() );
      vm->callFrame( *vm->param(0), 1 );
      return true;
   }
   else {
      if ( ! vm->regA().isOob() )
            mapped->append( vm->regA() );
   }

   vm->retval( mapped );
   return false;
}

/*#
   @function xmap
   @inset functional_support
   @brief Creates a new vector of items transforming each item in the original array through the mapping function, applying also filtering on undesired items.
   @param mfunc A function or sigma used to map the array.
   @param sequence A sequence to be mapped.
   @return The mapped sequence.

   @b mfunc is called iteratively for every item in the collection;  its return value is added to
   the mapped array. Moreover, each item in the collection is functionally evaluated before
   being passed to mfunc.

   The filter function may return an out of band nil item to signal that the current item should
   not be added to the final collection.

    In example:
   @code

      mapper = lambda item => (item < 0 ? oob(nil) : item ** 0.5)
      add = lambda a, b => a+b         // a lambda that will be evaluated

      inspect( xmap( mapper, [ [add, 99, 1], 4, -12, 9 ]) )    // returns [10, 2, 3]
   @endcode

   @see oob
   @see dolist
*/

FALCON_FUNC  core_xmap ( ::Falcon::VMachine *vm )
{
   Item *callable = vm->param(0);
   Item *i_origin = vm->param(1);
   if( callable == 0 || !callable->isCallable() ||
       i_origin == 0 || !i_origin->isArray()
      )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "C,A" ) ) );
      return;
   }

   CoreArray *origin = i_origin->asArray();
   CoreArray *mapped = new CoreArray( vm, origin->length() );
   if ( origin->length() > 0 )
   {
      vm->returnHandler( core_xmap_next );
      vm->addLocals( 3 );
      *vm->local(0) = (int64)1;
      *vm->local(1) = mapped;
      *vm->local(2) = (int64) 0;

      if ( vm->functionalEval( origin->at(0) ) )
      {
         return;
      }

      *vm->local(2) = (int64) 1;
      vm->pushParameter( vm->regA() );
      vm->callFrame( *vm->param(0), 1 );
      return;
   }

   vm->retval( mapped );
}

static bool core_filter_next ( ::Falcon::VMachine *vm )
{
   CoreArray *origin = vm->param(1)->asArray();
   CoreArray *mapped = vm->local(0)->asArray();
   uint32 count = (uint32) vm->local(1)->asInteger();

   if ( vm->regA().isTrue() )
      mapped->append( origin->at(count -1) );

   if( count == origin->length()  )
   {
      vm->retval( mapped );
      return false;
   }

   *vm->local(1) = (int64) count+1;
   vm->pushParameter( origin->at(count) );
   vm->callFrame( *vm->param(0), 1 );
   return true;
}


/*#
   @function filter
   @inset functional_support
   @brief Filters sequence using a filter function.
   @param ffunc A callable item used to filter the array.
   @param sequence A sequence of arbitrary items.
   @return The filtered sequence.

   ffunc is called iteratively for every item in the collection, which is passed as a parameter to it.
   If the call returns true, the item is added to the returned array; if it returns false,
   the item is not added.

   Items in the collection are treated literally (not evaluated).
*/

FALCON_FUNC  core_filter ( ::Falcon::VMachine *vm )
{
   Item *callable = vm->param(0);
   Item *i_origin = vm->param(1);
   if( callable == 0 || !callable->isCallable() ||
      i_origin == 0 || !i_origin->isArray()
      )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "C,A" ) ) );
      return;
   }

   CoreArray *origin = i_origin->asArray();
   CoreArray *mapped = new CoreArray( vm, origin->length() / 2 );
   if( origin->length() > 0 )
   {
      vm->returnHandler( core_filter_next );
      vm->addLocals(2);
      *vm->local(0) = mapped;
      *vm->local(1) = (int64) 1;
      vm->pushParameter( origin->at(0) );
      vm->callFrame( *vm->param(0), 1 );
      return;
   }

   vm->retval( mapped );
}


static bool core_reduce_next ( ::Falcon::VMachine *vm )
{
   // Callable in param 0
   CoreArray *origin = vm->param(1)->asArray();

   // if we had enough calls, return (the return value of the last call frame is
   // already what we want to return).
   uint32 count = (uint32) vm->local(0)->asInteger();
   if( count >= origin->length() )
      return false;

   // increment count for next call
   vm->local(0)->setInteger( count + 1 );

   // call next item
   vm->pushParameter( vm->regA() ); // last returned value
   vm->pushParameter( origin->at(count) ); // next element
   vm->callFrame( *vm->param(0), 2 );
   return true;
}

/*#
   @function reduce
   @inset functional_support
   @brief Uses the values in a given sequence and iteratively calls a reductor function to extract a single result.
   @param reductor A function or Sigma to reduce the array.
   @param sequence A sequence of arbitrary items.
   @optparam initial_value Optional startup value for the reduction.
   @return The reduced result.

   The reductor is a function receiving two values as parameters. The first value is the
   previous value returned by the reductor, while the second one is an item iteratively
   taken from the origin array. If a startup value is given, the first time the reductor
   is called that value is provided as its first parameter, otherwise the first two items
   from the array are used in the first call. If the collection is empty, the initial_value
   is returned instead, and if is not given, nil is returned. If a startup value is not given
   and the collection contains only one element, that element is returned.

   Some examples:
   @code
   > reduce( lambda a,b=> a+b, [1,2,3,4])       // sums 1 + 2 + 3 + 4 = 10
   > reduce( lambda a,b=> a+b, [1,2,3,4], -1 )  // sums -1 + 1 + 2 + 3 + 4 = 9
   > reduce( lambda a,b=> a+b, [1] )            // never calls lambda, returns 1
   > reduce( lambda a,b=> a+b, [], 0 )          // never calls lambda, returns 0
   > reduce( lambda a,b=> a+b, [] )             // never calls lambda, returns Nil
   @endcode

   Items in the collection are treated literally (not evaluated).
*/
FALCON_FUNC  core_reduce ( ::Falcon::VMachine *vm )
{
   Item *callable = vm->param(0);
   Item *i_origin = vm->param(1);
   Item *init = vm->param(2);
   if( callable == 0 || !callable->isCallable()||
      i_origin == 0 || !i_origin->isArray()
      )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "C,A,[X]" ) ) );
      return;
   }

   CoreArray *origin = i_origin->asArray();
   vm->addLocals(1);
   // local 0: array position

   if ( init != 0 )
   {
      if( origin->length() == 0 )
      {
         vm->retval( *init );
         return;
      }

      vm->returnHandler( core_reduce_next );
      vm->pushParameter( *init );
      vm->pushParameter( origin->at(0) );
      *vm->local(0) = (int64) 1;

      //WARNING: never use pre-cached item pointers after stack changes.
      vm->callFrame( *vm->param(0), 2 );
      return;
   }

   // if init == 0; if there is only one element in the array, return it.
   if ( origin->length() == 0 )
      vm->retnil();
   else if ( origin->length() == 1 )
      vm->retval( origin->at(0) );
   else
   {
      vm->returnHandler( core_reduce_next );
      *vm->local(0) = (int64) 2; // we'll start from 2

      // the first call is between the first and the second elements in the array.
      vm->pushParameter( origin->at(0) );
      vm->pushParameter( origin->at(1) );

      //WARNING: never use pre-cached item pointers after stack changes.
      vm->callFrame( *vm->param(0), 2 );
   }
}


static bool core_iff_next( ::Falcon::VMachine *vm )
{
   // anyhow, we don't want to be called anymore
   vm->returnHandler( 0 );

   if ( vm->regA().isTrue() )
   {
      if ( vm->functionalEval( *vm->param(1) ) )
         return true;
   }
   else
   {
      Item *i_ifFalse = vm->param(2);
      if ( i_ifFalse != 0 )
      {
         if ( vm->functionalEval( *i_ifFalse ) )
            return true;
      }
      else
         vm->retnil();
   }

   return false;
}


/*#
   @function iff
   @inset functional_support
   @brief Performs a functional if; if the first parameter evaluates to true, the second parameter is evaluated and then returned, else the third one is evaluated and returned.
   @param cfr A condition or a callable item.
   @param whenTrue Value to be called and/or returned in case cfr evaluates to true.
   @optparam whenFalse Value to be called and/or returned in case cfr evaluates to false.
   @return The evaluation result of one of the two branches (or nil).

   Basically, this function is meant to return the second parameter or the third (or nil if not given),
   depending on the value of the first parameter; however, every item is evaluated in a functional
   context. This means that cfr may be a callable item, in which case its return value will be evaluated
   for truthfulness, and also the other parameters may. In example:
   @code
      > iff( 0, "was true", "was false" )           // will print “was false”
      iff( [lambda a=>a*2, 1] , [printl, "ok!"] )   // will print “ok!” and return nil
   @endcode

   In the last example, we are not interested in the return value (printl returns nil), but in executing
   that item only in case the first item is true. The first item is a callable item too, so iff will first
   execute the given lambda, finding a result of 2 (true), and then will decide which element to pick, and
   eventually execute. Notice that:
   @code
      iff( 1 , printl( "ok!" ), printl( "no" ) )
   @endcode

   This would have forced Falcon to execute the two printl calls before entering the iff function;
   still, iff would have returned printl return values (which is nil in both cases).
*/
FALCON_FUNC  core_iff ( ::Falcon::VMachine *vm )
{
   Item *i_cond = vm->param(0);
   Item *i_ifTrue = vm->param(1);
   Item *i_ifFalse = vm->param(2);

   if( i_cond == 0 || i_ifTrue == 0 )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "X,X,[X]" ) ) );
      return;
   }

   // we can use pre-fetched values as we have stack unchanged on
   // paths where we use item pointers.

   vm->returnHandler( core_iff_next );
   if ( vm->functionalEval( *i_cond ) )
   {
      return;
   }
   vm->returnHandler( 0 );

   if ( vm->regA().isTrue() )
   {
      vm->functionalEval( *i_ifTrue );
   }
   else {
      if ( i_ifFalse != 0 )
         vm->functionalEval( *i_ifFalse );
      else
         vm->retnil();
   }
}


static bool core_choice_next( ::Falcon::VMachine *vm )
{
   if ( vm->regA().isTrue() )
   {
      vm->retval( *vm->param(1) );
   }
   else {
      Item *i_ifFalse = vm->param(2);
      if ( i_ifFalse != 0 )
         vm->retval( *i_ifFalse );
      else
         vm->retnil();
   }

   return false;
}

/*#
   @function choice
   @inset functional_support
   @brief Selects one of two alternatives depending on the evaluation of the first parameter.
   @param selector The item to be evaluated.
   @param whenTrue The item to return if selector evaluates to true.
   @optparam whenFalse The item to be returned if selector evaluates to false
   @optparam ... Optional parameters to be passed to the first callable item.
   @return The return value of the last callable item.

   The selector parameter is evaluated in functional context. If it's a true atom or if it's a
   callable array which returns a true value, the ifTrue parameter is returned as-is, else the
   ifFalse parameter is returned. If the ifFalse parameter is not given and the selector evaluates
   to false, nil is returned.

   The choice function is equivalent to iff where each branch is passed through the @a lit function:
   @code
      choice( selector, a, b ) == iff( selector, [lit, a], [lit, b] )
   @endcode
   In case a literal value is needed, choice is more efficient than using iff and applying lit on
   the parameters.
*/
FALCON_FUNC  core_choice ( ::Falcon::VMachine *vm )
{
   Item *i_cond = vm->param(0);
   Item *i_ifTrue = vm->param(1);
   Item *i_ifFalse = vm->param(2);

   if( i_cond == 0 || i_ifTrue == 0 )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "X,X,[X]" ) ) );
      return;
   }

   vm->returnHandler( core_choice_next );
   if ( vm->functionalEval( *i_cond ) )
   {
      return;
   }
   vm->returnHandler( 0 );

   if ( vm->regA().isTrue() )
   {
      vm->retval( *i_ifTrue );
   }
   else {
      if ( i_ifFalse != 0 )
         vm->retval( *i_ifFalse );
      else
         vm->retnil();
   }
}

/*#
   @function lit
   @inset functional_support
   @brief Return its parameter as-is
   @param item A condition or a callable item.
   @return The parameter unevaluated.

   This function is meant to interrupt functional evaluation of lists. It has
   the same meaning of the single quote literal ' operator of the LISP language.

   In example, the following code will return either a callable instance of printl,
   which prints a “prompt” before the parameter, or a callable instance of inspect:
   @code
      iff( a > 0, [lit, [printl, "val: "] ], inspect)( param )
   @endcode
   as inspect is a callable token, but not an evaluable one, it is already returned literally;
   however, [printl, “val:”] would be considered an evaluable item. To take its literal
   value and prevent evaluation in functional context, the lit construct must be used.
*/

FALCON_FUNC  core_lit ( ::Falcon::VMachine *vm )
{
   Item *i_cond = vm->param(0);

   if( i_cond == 0 )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "X" ) ) );
      return;
   }

   vm->regA() = *i_cond;
   // result already in A.
}


static bool core_cascade_next ( ::Falcon::VMachine *vm )
{
   // Param 0: callables array
   // local 0: counter (position)
   // local 1: last accepted result
   CoreArray *callables = vm->param(0)->asArray();
   uint32 count = (uint32) vm->local(0)->asInteger();

   // Done?
   if ( count >= callables->length() )
   {
      // if the last result is not accepted, return last accepted
      if ( vm->regA().isOob() )
      {
         // reset OOB, that may be set on first unaccepted parameter.
         vm->local(1)->resetOob();
         vm->retval( *vm->local(1) );
      }
      // else, just keep
      return false;
   }

   uint32 pc;

   // still some loop to do
   // accept result?
   if ( vm->regA().isOob() )
   {
      // not accepted.

      // has at least one parameter been accepted?
      if ( vm->local(1)->isOob() )
      {
         // no? -- replay initial params
         pc = vm->paramCount();
         for ( uint32 pi = 1; pi < pc; pi++ )
         {
            vm->pushParameter( *vm->param(pi) );
         }
         pc--;  //first param is our callable
      }
      else {
         // yes? -- reuse last accepted parameter
         pc = 1;
         vm->pushParameter( *vm->local(1) );
      }
   }
   else {
      *vm->local(1) = vm->regA();
      pc = 1;
      vm->pushParameter( vm->regA() );
   }

   // prepare next call
   vm->local(0)->setInteger( count + 1 );

   // perform call
   if ( ! vm->callFrame( callables->at(count), pc ) )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_non_callable ) ) );
      return false;
   }

   return true;
}

/*#
   @function cascade
   @inset functional_support
   @brief Concatenate a set of callable items so to form a single execution unit.
   @param callList Sequence of callable items.
   @optparam ... Optional parameters to be passed to the first callable item.
   @return The return value of the last callable item.

   This function executes a set of callable items passing the parameters it receives
   beyond the first one to the first  item in the list; from there on, the return value
   of the previous call is fed as the sole parameter of the next call. In other words,
   @code
      cascade( [F1, F2, ..., FN], p1, p2, ..., pn )
   @endcode
   is equivalent to
   @code
      FN( ... F2( F1( p1, p2, ..., pn ) ) ... )
   @endcode

   A function may declare itself “uninterested” to insert its value in the cascade
   by returning an out-of-band item. In that case, the return value is ignored and the same parameter
   it received is passed on to the next calls and eventually returned.

   Notice that the call list is not evaluated in functional context; it is just a list
   of callable items. To evaluate the list, or part of it, in functional context, use
   the eval() function.

   A simple example usage is the following:
   @code
      function square( a )
         return a * a
      end

      function sqrt( a )
         return a ** 0.5
      end

      cascade_abs = [cascade, [square, sqrt] ]
      > cascade_abs( 2 )      // 2
      > cascade_abs( -4 )     // 4
   @endcode

   Thanks to the possibility to prevent insertion of the return value in the function call sequence,
   it is possible to program “interceptors” that will catch the progress of the sequence without
   interfering:

   @code
      function showprog( v )
         > "Result currently ", v
        return oob(nil)
      end

      // define sqrt and square as before...
      cascade_abs = [cascade, [square, showprog, sqrt, showprog] ]
      > "First process: ", cascade_abs( 2 )
      > "Second process: ", cascade_abs( -4 )
   @endcode

   If the first function of the list declines processing by returning an oob item, the initial parameters
   are all passed to the second function, and so on till the last call.

   In example:

   @code
      function whichparams( a, b )
         > "Called with ", a, " and ", b
         return oob(nil)
      end

      csq = [cascade, [ whichparams, lambda a,b=> a*b] ]
      > csq( 3, 4 )
   @endcode

   Here, the first function in the list intercepts the parameters but, as it doesn't
   accepts them, they are both passed to the second in the list.

   @see oob
*/
FALCON_FUNC  core_cascade ( ::Falcon::VMachine *vm )
{
   Item *i_callables = vm->param(0);

   if( i_callables == 0 || !i_callables->isArray() )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "A,..." ) ) );
      return;
   }

   // for the first callable...
   CoreArray *callables = i_callables->asArray();
   if( callables->length() == 0 )
   {
      vm->retnil();
      return;
   }

   // we have at least one callable.
   // Prepare the local space
   // 0: array counter
   // 1: saved previous value
   // saved previous value is initialized to oob until
   // someone accepts the first parameters.
   vm->addLocals(2);
   vm->local(0)->setInteger( 1 );  // we'll start from 1
   vm->local(1)->setOob();

   // echo the parameters to the first call
   uint32 pcount = vm->paramCount();
   for ( uint32 pi = 1; pi < pcount; pi++ )
   {
      vm->pushParameter( *vm->param(pi) );
   }
   pcount--;

   // install the handler
   vm->returnHandler( core_cascade_next );

   // perform the real call
   if ( ! vm->callFrame( callables->at(0), pcount ) )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_non_callable ) ) );
   }
}


static bool core_floop_next ( ::Falcon::VMachine *vm )
{
   // Param 0: callables array
   CoreArray *callables = vm->param(0)->asArray();
   // local 0: counter (position)
   uint32 count = (uint32) vm->local(0)->asInteger();

   // next item.
   ++count;

   // still some loop to do
   if ( vm->regA().isInteger() && vm->regA().isOob() )
   {
      if ( vm->regA().asInteger() == 0 )
      {
         // we're done.
         vm->returnHandler( 0 ); // ensure we're not called after first loop
         vm->retnil();
         return false;
      }
      else if ( vm->regA().asInteger() == 1 )
      {
         // continue
         count = 0;
      }
   }

   if ( count >= callables->length() )
   {
      count = 0;
   }

   // save the count
   *vm->local(0) = (int64) count;
   // find a callable in the array
   if ( ! vm->callFrame( (*callables)[count], 0 ) )
   {
      // set the item as A and recall ourself for evaluation
      vm->regA() = (*callables)[count];
      vm->recallFrame();
      return true;
   }

   // else, just return true
   return true;
}

/*#
   @function floop
   @inset functional_support
   @brief Repeats indefinitely a list of operations.
   @param sequence A sequence of callable items that gets called one after another.

   Every item in @b sequence gets executed, one after another. When the last element is executed,
   the first one is called again, looping indefinitely.
   Any function in the sequence may interrupt the loop by returning an out-of-band 0;
   if a function returns an out of band 1, all the remaining items in the list are ignored
   and the loop starts again from the first item.

   Items in the array are not functionally evaluated.
*/

FALCON_FUNC  core_floop ( ::Falcon::VMachine *vm )
{
   Item *i_callables = vm->param(0);

   if( i_callables == 0 || !i_callables->isArray() )
   {
      vm->raiseRTError( new ParamError( ErrorParam( e_inv_params ).
         extra( "A" ) ) );
      return;
   }

   // for the first callable...
   CoreArray *callables = i_callables->asArray();
   if( callables->length() == 0 )
   {
      return;
   }

   // we have at least one callable.
   // Prepare the local space
   // 0: array counter
   vm->addLocals(1);
   vm->local(0)->setInteger( callables->length() );  // we'll start from 0 from the first loop

   // install the handler
   vm->returnHandler( core_floop_next );

   // call it directly
   vm->regA().setNil(); // zero to avoid false signals to next handler
   vm->callFrameNow( core_floop_next );
}

/*#
   @function firstOf
   @inset functional_support
   @brief Returns the first non-false of its parameters.
   @param ... Any number of arbitrary parameters.
   @return The first non-false item.

   This function scans the paraters one at a time. Sigma evaluation is stopped,
   or in other words, every parameters is considered as-is, as if @a lit was used on each of them.
   The function returns the first parameter being non-false in a standard Falcon truth check.
   Nonzero numeric values, non empty strings, arrays and dictionaries and any object is considered true.

   If none of the parameters is true, of is none of the parameter is given, the function returns nil
   (which is considered  false).
*/
FALCON_FUNC  core_firstof ( ::Falcon::VMachine *vm )
{
   int count = 0;
   Item *i_elem = vm->param(0);
   while( i_elem != 0 )
   {
      if ( i_elem->isTrue() )
      {
         vm->retval( *i_elem );
         return;
      }
      i_elem = vm->param( ++count );
   }

   vm->retnil();
}

}
}

/* end of functional_ext.cpp */