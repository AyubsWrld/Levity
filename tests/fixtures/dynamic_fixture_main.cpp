extern "C" auto lgpl_fixture_value() -> int;

auto main() -> int { return lgpl_fixture_value() == 42 ? 0 : 1; }
