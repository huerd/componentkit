/*
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#import <RenderCore/CKDefines.h>

#if CK_NOT_SWIFT

#import <string>
#import <unordered_map>

#import <UIKit/UIKit.h>

#import <RenderCore/ComponentViewReuseUtilities.h>
#import <RenderCore/CKAccessibilityContext.h>
#import <RenderCore/CKComponentViewAttribute.h>
#import <RenderCore/CKComponentViewClass.h>
#import <RenderCore/CKContainerWrapper.h>

typedef void (^CKComponentViewReuseBlock)(UIView *);

/**
 A CKViewConfiguration specifies the class of a view and the attributes that should be applied to it.
 Initialize a configuration with brace syntax, for example:

 {[UIView class]}
 {[UIView class], {{@selector(setBackgroundColor:), [UIColor redColor]}, {@selector(setAlpha:), @0.5}}}
 */
struct CKViewConfiguration {

  CKViewConfiguration() noexcept :
    rep(singletonViewConfiguration()) {}

  CKViewConfiguration(CKComponentViewClass &&cls) noexcept :
    CKViewConfiguration(std::move(cls), {}) {}

  // Prefer overloaded constructors to default arguments to prevent code bloat; with default arguments
  // the compiler must insert initialization of each default value inline at the callsite.
  CKViewConfiguration(CKComponentViewClass &&cls,
                      CKContainerWrapper<CKViewComponentAttributeValueMap> &&attrs) noexcept :
    CKViewConfiguration(std::move(cls), std::move(attrs), {}) {}

  CKViewConfiguration(CKComponentViewClass &&cls,
                      CKContainerWrapper<CKViewComponentAttributeValueMap> &&attrs,
                      CKAccessibilityContext &&accessibilityCtx,
                      bool blockImplicitAnimations = false) noexcept
  {
    // Need to use attrs before we move it below.
    CKViewComponentAttributeValueMap attrsMap = attrs.take();
    CK::Component::PersistentAttributeShape attributeShape(attrsMap);
    rep.reset(new Repr({
      .viewClass = std::move(cls),
      .attributes = std::make_shared<CKViewComponentAttributeValueMap>(std::move(attrsMap)),
      .accessibilityContext = std::move(accessibilityCtx),
      .attributeShape = std::move(attributeShape),
      .blockImplicitAnimations = blockImplicitAnimations
    }));
  }

  ~CKViewConfiguration() {}

  const CKComponentViewClass &viewClass() const noexcept
  {
    return rep->viewClass;
  }

  std::shared_ptr<const CKViewComponentAttributeValueMap> attributes() const noexcept
  {
    return rep->attributes;
  }

  const CKAccessibilityContext &accessibilityContext() const noexcept
  {
    return rep->accessibilityContext;
  }

  BOOL isDefaultConfiguration() const
  {
    return rep == singletonViewConfiguration();
  }

  bool blockImplicitAnimations() const noexcept
  {
    return rep->blockImplicitAnimations;
  }

  const CK::Component::PersistentAttributeShape &attributeShape() const noexcept
  {
    return rep->attributeShape;
  }

private:
  struct Repr {
    CKComponentViewClass viewClass;
    std::shared_ptr<const CKViewComponentAttributeValueMap> attributes;
    CKAccessibilityContext accessibilityContext;
    CK::Component::PersistentAttributeShape attributeShape;
    bool blockImplicitAnimations;
  };

  std::shared_ptr<const Repr> rep; // const is important for the singletonViewConfiguration optimization.

  // It would be ideal to use std::unique_ptr here and give this class move semantics, but it already has value semantics
  // and there are a few complicated flows.
  std::shared_ptr<const CKViewConfiguration::Repr> singletonViewConfiguration() const
  {
    static std::shared_ptr<const CKViewConfiguration::Repr> p = CKViewConfiguration(CKComponentViewClass()).rep;
    return p;
  }
};

namespace std {
  template <>
  struct hash<CKViewConfiguration>
  {
    size_t operator()(const CKViewConfiguration &cl) const noexcept
    {
      NSUInteger subhashes[] = {
        std::hash<CKComponentViewClass>()(cl.viewClass()),
        std::hash<CKViewComponentAttributeValueMap>()(*cl.attributes()),
        std::hash<bool>()(cl.blockImplicitAnimations()),
      };
      return CKIntegerArrayHash(subhashes, std::end(subhashes) - std::begin(subhashes));
    }
  };
}

#endif
