#pragma once

#include <functional>

namespace ccache {

class Finalizer {
  public:
    Finalizer(std::function<void()> finalizer);
    ~Finalizer();

  private:
    std::function<void()> m_finalizer;
};

inline Finalizer::Finalizer(std::function<void()> finalizer)
    : m_finalizer(finalizer) {
}

inline Finalizer::~Finalizer() {
    m_finalizer();
}

}  // namespace ccache

