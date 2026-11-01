#pragma once

#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>

using namespace sts1cobcsw::fs;

auto PrintWorkingDirectory() -> void;
auto ListDirectory(Path const & path) -> void;
auto CreateDirectory(Path const & path) -> Result<void>;
auto RemoveObject(Path const & path) -> Result<void>;
auto ChangeDirectory(Path const & path) -> Result<void>;
auto Touch(Path const & path) -> Result<void>;