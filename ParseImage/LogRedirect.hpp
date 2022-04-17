#pragma once
#include <make_exception.hpp>

#include <fstream>
#include <iostream>

// Bitfield representing redirectable standard streams. Used by the LogRedirect object to specify which streams to redirect.
struct StandardStream {
private:
	int _v;
public:
	constexpr StandardStream(const int& v) : _v{ v } {}

	constexpr operator int() const { return _v; }

	constexpr bool contains(const int& v) const { return (_v & v) != 0; }

	constexpr StandardStream& operator|=(const int& v) { _v |= v; return *this; }
	constexpr StandardStream& operator&=(const int& v) { _v &= v; return *this; }
	constexpr StandardStream& operator^=(const int& v) { _v ^= v; return *this; }

	static const StandardStream NONE, STDOUT, STDERR, STDLOG, ALL;
};
inline const constexpr StandardStream StandardStream::NONE{ 0 };
inline const constexpr StandardStream StandardStream::STDOUT{ 1 };
inline const constexpr StandardStream StandardStream::STDERR{ 2 };
inline const constexpr StandardStream StandardStream::STDLOG{ 4 };
inline const constexpr StandardStream StandardStream::ALL{ StandardStream::STDOUT | StandardStream::STDERR | StandardStream::STDLOG };

// wrapper object that redirects all output streams to the specified streambuf
class LogRedirect {
	std::streambuf* out{ nullptr }, * err{ nullptr }, * log{ nullptr };
	std::ofstream ofs;
public:
	LogRedirect() = default;
	~LogRedirect() noexcept
	{
		if (out != nullptr)
			std::cout.rdbuf(out);
		if (err != nullptr)
			std::cerr.rdbuf(err);
		if (log != nullptr)
			std::clog.rdbuf(log);
		if (ofs.is_open())
			ofs.close();
	}

	// check if any of the specified streams are redirected
	[[nodiscard]] bool anyRedirected(const StandardStream& targets) const
	{
		return ((targets.contains(StandardStream::STDOUT) && out != nullptr) || (targets.contains(StandardStream::STDERR) && err != nullptr) || (targets.contains(StandardStream::STDLOG) && log != nullptr));
	}
	// check if all of the specified streams are redirected
	[[nodiscard]] bool allRedirected(const StandardStream& targets) const
	{
		return (!targets.contains(StandardStream::STDOUT) || out != nullptr) && (!targets.contains(StandardStream::STDERR) || err != nullptr) && (targets.contains(StandardStream::STDLOG) || log != nullptr);
	}

	// returns streams that were successfully redirected
	StandardStream redirect(const StandardStream& targets, std::streambuf* sbuf)
	{
		auto ret{ StandardStream::NONE };
		if (targets.contains(StandardStream::STDOUT)) {
			if (out = std::cout.rdbuf(sbuf)) // redirect STDOUT
				ret |= StandardStream::STDOUT;
		}
		if (targets.contains(StandardStream::STDERR)) {
			if (err = std::cerr.rdbuf(sbuf)) // redirect STDERR
				ret |= StandardStream::STDERR;
		}
		if (targets.contains(StandardStream::STDLOG)) {
			if (log = std::clog.rdbuf(sbuf)) // redirect STDLOG
				ret |= StandardStream::STDLOG;
		}
		return ret;
	}
	// returns streams that were successfully redirected
	StandardStream redirect(const StandardStream& targets, const std::string& file, const std::ios_base::openmode& mode = 2) noexcept(false)
	{
		ofs.open(file, mode);
		if (ofs.is_open())
			return redirect(targets, ofs.rdbuf());
		else throw make_exception("Failed to open log output file '", file, '\'');
	}
	StandardStream reset(const StandardStream& targets) noexcept
	{
		auto ret{ StandardStream::NONE };
		try {
			if (targets.contains(StandardStream::STDOUT) && out != nullptr) { // reset STDOUT
				std::cout.rdbuf(out);
				out = nullptr;
				ret |= StandardStream::STDOUT;
			}
			if (targets.contains(StandardStream::STDERR) && err != nullptr) { // reset STDERR
				std::cerr.rdbuf(err);
				err = nullptr;
				ret |= StandardStream::STDERR;
			}
			if (targets.contains(StandardStream::STDLOG) && log != nullptr) { // reset STDLOG
				std::clog.rdbuf(log);
				log = nullptr;
				ret |= StandardStream::STDLOG;
			}
		} catch (...) {}
		return ret;
	}
};
