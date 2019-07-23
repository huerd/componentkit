/*
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#import "CKComponentViewClass.h"

#import <ComponentKit/CKAssert.h>

#import "CKInternalHelpers.h"
#import "CKComponentViewConfiguration.h"
#import "CKGlobalConfig.h"

std::string CKComponentViewClassIdentifier::description() const
{
  switch (this->identifierType) {
    case EMPTY_IDENTIFIER:
      return "";
    case CLASS_BASED_IDENTIFIER:
      return std::string(((const char *)this->ptr1)) + "-" + (const char *)this->ptr2 + "-" + (const char *)this->ptr3;
    case FUNCTION_BASED_IDENTIFIER:
      return CKStringFromPointer((const void *)this->ptr1);
    case STRING_BASED_IDENTIFIER:
      return (const char *)this->ptr1;
  }
}

static CKComponentViewReuseBlock blockFromSEL(SEL sel) noexcept
{
  if (sel) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
    return ^(UIView *v){ [v performSelector:sel]; };
#pragma clang diagnostic pop
  }
  return nil;
}

static CKComponentViewFactoryBlock viewFactoryFromViewClass(Class viewClass) noexcept
{
  CKCAssert([viewClass isSubclassOfClass:[UIView class]], @"%@ is not a subclass of UIView", viewClass);
  // Passing a nil `viewClass` is unexpected. We should return a nil view factory and treat this as a viewless component.
  // Otherwise nil will be returned by view factory and it will crash.
  if (viewClass) {
    return ^{ return [[viewClass alloc] init]; };
  } else {
    return nil;
  }
}

CKComponentViewClass::CKComponentViewClass() noexcept : usingStringIdentifier(CKReadGlobalConfig().enableComponentViewClassIdentifier == false), factory(nil)
{
}

CKComponentViewClass::CKComponentViewClass(Class viewClass) noexcept :
factory(viewFactoryFromViewClass(viewClass))
{
  if (CKReadGlobalConfig().enableComponentViewClassIdentifier) {
    identifier = { viewClass };
  } else {
    usingStringIdentifier = true;
    stringIdentifier = std::string(class_getName(viewClass));
  }
}

CKComponentViewClass::CKComponentViewClass(Class viewClass, SEL enter, SEL leave) noexcept :
factory(viewFactoryFromViewClass(viewClass)),
didEnterReusePool(blockFromSEL(enter)),
willLeaveReusePool(blockFromSEL(leave))
{
  if (CKReadGlobalConfig().enableComponentViewClassIdentifier) {
    identifier = { viewClass, enter, leave };
  } else {
    usingStringIdentifier = true;
    stringIdentifier = std::string(class_getName(viewClass)) + "-" + sel_getName(enter) + "-" + sel_getName(leave);
  }
}

CKComponentViewClass::CKComponentViewClass(CKComponentViewFactoryFunc fact,
                                           void (^enter)(UIView *),
                                           void (^leave)(UIView *)) noexcept
: factory(^UIView*(void) {return fact();}), didEnterReusePool(enter), willLeaveReusePool(leave)
{
  if (CKReadGlobalConfig().enableComponentViewClassIdentifier) {
    identifier = { fact };
  } else {
    usingStringIdentifier = true;
    stringIdentifier = CKStringFromPointer((const void *)fact);
  }
}

CKComponentViewClass::CKComponentViewClass(const std::string &i,
                                           CKComponentViewFactoryBlock fact,
                                           void (^enter)(UIView *),
                                           void (^leave)(UIView *)) noexcept
: factory(fact), didEnterReusePool(enter), willLeaveReusePool(leave)
{
#if DEBUG
  CKCAssertNil(objc_getClass(i.c_str()), @"You may not use a class name as the identifier; it would conflict with "
               "the constructor variant that takes a viewClass.");
#endif
  if (CKReadGlobalConfig().enableComponentViewClassIdentifier) {
    identifier = { i.c_str() };
  } else {
    usingStringIdentifier = true;
    stringIdentifier = i;
  }
}

UIView *CKComponentViewClass::createView() const
{
  return factory ? factory() : nil;
}

BOOL CKComponentViewClass::hasView() const
{
  return factory != nil;
}