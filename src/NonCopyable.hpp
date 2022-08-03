//
//  NonCopyable.hpp
//  ccache
//
//  Created by king on 2022/8/3.
//

#ifndef NonCopyable_h
#define NonCopyable_h

namespace ccache {

class NonCopyable {
  public:
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;

  protected:
    NonCopyable() = default;
};

};     // namespace ccache
#endif /* NonCopyable_h */

