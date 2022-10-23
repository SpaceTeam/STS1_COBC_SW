#pragma once


namespace sts1cobcsw::periphery
{
// This is just a dummy implementation now that does not store anything in a persistent memory.
template<typename T>
class PersistentState
{
  public:
    PersistentState() = delete;
    explicit PersistentState(T const & t);
    ~PersistentState() = default;
    PersistentState(PersistentState const &) = delete;
    auto operator=(PersistentState const &) -> PersistentState & = delete;
    PersistentState(PersistentState &&) = delete;
    auto operator=(PersistentState &&) -> PersistentState & = delete;

    [[nodiscard]] auto Get() const -> T;
    auto Set(T const & t) -> void;

  private:
    T value_;
};


template<typename T>
PersistentState<T>::PersistentState(T const & t) : value_{t}
{
}


template<typename T>
auto PersistentState<T>::Get() const -> T
{
    return value_;
}


template<typename T>
auto PersistentState<T>::Set(T const & t) -> void
{
    value_ = t;
}
}