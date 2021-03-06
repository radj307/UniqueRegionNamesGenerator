#include "LogRedirect.hpp"
#include "output_operators.hpp"
#include "PartitionStats.hpp"
#include "config.hpp"
#include "ImageWrapper.hpp"
#include "RegionStatsMap.hpp"

#include <TermAPI.hpp>
#include <ParamsAPI2.hpp>
#include <env.hpp>
#include <envpath.hpp>
#include <fileio.hpp>
#include <INIRedux.hpp>

#include <opencv2/opencv.hpp>

/**
 * @brief				Parse a given string by splitting it with one of the given delimiters, then converting both sides to integral types.
 * @tparam RetType		Either a `cv::Point` or `cv::Size` type.
 * @param s				Input String
 * @param seperators	List of characters that are valid delimiters.
 * @returns				RetType
 */
template<var::any_same<cv::Point, cv::Size> RetType>
RetType parse_string(const std::string& s, const std::string& seperators = ":,")
{
	const auto& [xstr, ystr] { str::split(s, seperators) };
	if (std::all_of(xstr.begin(), xstr.end(), isdigit) && std::all_of(ystr.begin(), ystr.end(), isdigit))
		return RetType{ str::stoi(xstr), str::stoi(ystr) };
	else throw make_exception("Cannot parse string '", s, "' into a valid pair of integrals!");
}


/// @brief	Translates index coordinates (origin 0,0 top-left) to cell coordinates (origin -74, 49 top-left)
cv::Point offsetCellCoordinates(const cv::Point& p, const cv::Point& pMin = { 0, 0 }, const cv::Point& pMax = { 149, 99 })
{
	const cv::Point cellMin{ -74, 49 }, cellMax{ 75, -50 };

	const auto& translateAxis{ [](const auto& v, const auto& oldMin, const auto& oldMax, const auto& newMin, const auto& newMax) {
		if (oldMin == oldMax || newMin == newMax)
			throw make_exception("Invalid translation: ( ", oldMin, " - ", oldMax, " ) => ( ", newMin, " - ", newMax, " )");
		const auto
			& oldRange{ oldMax - oldMin },
			& newRange{ newMax - newMin };
		return (((v - oldMin) * newRange) / oldRange) + newMin;
	} };

	return{
		translateAxis(p.x, pMin.x, pMax.x, cellMin.x, cellMax.x),
		translateAxis(p.y, pMin.y, pMax.y, cellMin.y, cellMax.y)
	};
}

inline std::ostream& operator<<(std::ostream& os, const RGB& rgb)
{
	return os << str::fromBase10(rgb.r(), 16) << str::fromBase10(rgb.g(), 16) << str::fromBase10(rgb.b(), 16);
}

inline void ValidateRegionVec(RegionVec const& regionVec)
{
	std::map<RGB, std::vector<Region>> color_errors;
	std::map<std::string, std::vector<Region>> name_errors;

	for (size_t i{ 0ull }; i < regionVec.size(); ++i) {
		const auto& here{ regionVec.at(i) };
		for (size_t j{ 0ull }; j < regionVec.size(); ++j) {
			const auto& o{ regionVec.at(j) };
			if (j != i && here.id != o.id) {
				if (here.color == o.color) {
					if (color_errors[here.color].empty())
						color_errors[here.color].emplace_back(here);
					color_errors[here.color].emplace_back(o);
				}
				if (here.mapName == o.mapName) {
					if (name_errors[here.mapName].empty())
						name_errors[here.mapName].emplace_back(here);
					name_errors[here.mapName].emplace_back(o);
				}
			}
		}
	}

	if (color_errors.empty() && name_errors.empty())
		return;

	for (const auto& [color, regions] : color_errors)
		std::cerr << term::get_error() << "Color '" << color << "' is assigned to multiple regions! " << regions << std::endl;
	for (const auto& [name, regions] : name_errors)
		std::cerr << term::get_error() << "Map Name '" << name << "' is assigned to multiple regions! " << regions << std::endl;

	throw make_exception("One or more regions have identical mapping data, the generator cannot continue!");
}

int main(const int argc, char** argv)
{
	using CLK = std::chrono::high_resolution_clock;

	try {
		opt::ParamsAPI2 args{ argc, argv, 'f', "file", 'd', "dim", 'T', "timeout", 'o', "out", 't', "threshold", 'i', "ini", 'w', "worldspace" };
		env::PATH PATH;
		const auto& [myPath, myName] { PATH.resolve_split(argv[0]) };

		// show help
		if (args.empty() || args.check_any<opt::Flag, opt::Option>('h', "help")) {
			std::cout
				<< "ParseImage Usage:\n"
				<< "  " << std::filesystem::path(myName).replace_extension().generic_string() << " <OPTIONS>" << '\n'
				<< '\n'
				<< "OPTIONS:\n"
				<< "  -h  --help              Shows this usage guide.\n"
				<< "  -f  --file <PATH>       Specify an image to load.\n"
				<< "  -o  --out <PATH>        Specify a directory to export the results to.\n"
				<< "  -d  --dim <X:Y>         Specify the image partition dimensions that the input image is divided into.\n"
				<< "      --display           Displays each partition in a window while parsing.\n"
				<< "  -T  --timeout <ms>      When '--display' is specified, closes the display window after '<ms>' milliseconds.\n"
				<< "                           a value of 0 will wait forever, which is the default behaviour.\n"
				<< "  -t  --threshold <%>     A percentage in the range (0 - 100) that determines the minimum number of matching\n"
				<< "                           pixels that a partition must have in order for it to be considered part of a region.\n"
				<< "                           Setting this to `0` will NOT add any regions that don't have at least 1 pixel present!"
				<< " -i  --ini <PATH>         Specify the location of the INI config file. Default is the current working directory, named 'regions.ini'\n"
				<< " -w  --worldspace <NAME>  Specify the filename (not extension) of the output files.\n"
				;
		}

		file::MINI ini;

		if (const auto& path{ myPath / "regions.ini" }; file::exists(path)) {
			std::clog << "Reading region config at '" << path.generic_string() << "'.\n";
			ini.read(path);
		}

		for (const auto& it : args.typegetv_all<opt::Flag, opt::Option>('i', "ini")) {
			if (!file::exists(it))
				throw make_exception("Filepath '", it, "' doesn't exist!");
			else {
				std::clog << "Reading region config at '" << it << "'.\n";
				ini.read(it);
			}
		}

		if (ini.empty())
			throw make_exception("Failed to retrieve any valid data from the provided INI config files!");

		const auto& regionMap{ cfg::getRegions(ini) };
		ValidateRegionVec(regionMap);
		std::cout << "Successfully validated the region config." << std::endl;
		const ColorMap colormap{ regionMap };

		if (const auto& fileArg{ args.typegetv_any<opt::Flag, opt::Option>('f', "file") }; fileArg.has_value()) {
			std::filesystem::path path{ fileArg.value() };

			if (!file::exists(path)) // if the path doesn't exist as-is, attempt to resolve it using the PATH variable.
				path = PATH.resolve(path, { (path.has_extension() ? path.extension().generic_string() : ""), ".png", ".jpg", ".bmp" });

			// Keypress timeout for OpenCV display windows
			const int windowTimeout{ args.castgetv_any<int, opt::Flag, opt::Option>(str::stoi, 'T', "timeout").value_or(0) };
			// Percentage of pixels required to return a region (region must have at least 1 pixel to be detected by the parser, this is applied after parsing)
			const float pxThreshold{ args.castgetv_any<float, opt::Flag, opt::Option>([](std::string&& str) -> float {
				if (std::all_of(std::forward<std::string>(str).begin(), std::forward<std::string>(str).end(), isdigit))
					return static_cast<float>(str::stoi(std::move(str))) / 100.0f;
				else throw make_exception("Invalid threshold value '", str, "' contains invalid characters! (Only digits are allowed)");
			}, 't', "threshold").value_or(0.0f) };

			std::clog
				<< "Window Timeout:   " << color::setcolor::green << windowTimeout << color::setcolor::reset << '\n'
				<< "Pixel Threshold:  " << color::setcolor::green << pxThreshold << " / 1.0" << color::setcolor::reset << "  ( " << color::setcolor::green << pxThreshold * 100.0f << '%' << color::setcolor::reset << " )\n";

			std::filesystem::path logpath{ "OpenCV.log" };

			if (file::exists(path)) {
				LogRedirect streams;
				streams.redirect(StandardStream::STDOUT | StandardStream::STDERR, logpath.generic_string());
				std::clog << "Redirected " << color::setcolor::red << "STDOUT" << color::setcolor::reset << " & " << color::setcolor::red << "STDERR" << color::setcolor::reset << " to logfile:  " << logpath << '\n';

				if (ImageWrapper img{ path.generic_string() }; img.loaded()) {
					std::clog << "Successfully loaded image file '" << path << '\'' << std::endl;

					if (const auto& dimArg{ args.typegetv_any<opt::Flag, opt::Option>('d', "dim") }; dimArg.has_value()) {
						cv::Size partSize = parse_string<cv::Size>(dimArg.value(), ":,");
						std::clog << "Partition cv::Size:  [ " << partSize.width << " x " << partSize.height << " ]\n";

						const int& cols{ img.image.cols / partSize.width };
						const int& rows{ img.image.rows / partSize.height };

						bool display_each{ args.checkopt("display") };
						const std::string windowName{ "Display" };

						if (display_each)
							cv::namedWindow(windowName); // open a window

						RegionStatsMap regionStats;

						HoldMap vec;
						vec.reserve(static_cast<size_t>(cols * rows));
						size_t i = 0;

						const auto t_start{ CLK::now() };

						for (int y{ 0 }; y < rows; ++y) {
							unsigned row_count{ 0u };
							for (int x{ 0 }; x < cols; ++x, ++i) {
								const auto& rect{ cv::Rect(x * partSize.width, y * partSize.height, partSize.width, partSize.height) };
								auto part{ img.image(rect) };
								const auto& cellPos{ offsetCellCoordinates(cv::Point{ x, y }) };
								std::clog << "Processing Partition #" << color::setcolor::green << i << color::setcolor::reset << '\n'
									<< "  Partition Index:   ( " << color::setcolor::yellow << x << color::setcolor::reset << ", " << color::setcolor::yellow << y << color::setcolor::reset << " )\n"
									<< "  Cell Coordinates:  ( " << color::setcolor::yellow << cellPos.x << color::setcolor::reset << ", " << color::setcolor::yellow << cellPos.y << color::setcolor::reset << " )\n";
								if (display_each) {
									cv::imshow(windowName, part); // display the image in the window
									cv::waitKey(windowTimeout);
								}
								if (PartitionStats stats(std::move(part), colormap); stats.valid() && !stats.empty()) {
									if (auto regions{ stats.getRegions(pxThreshold) }; !regions.empty()) {
										std::clog << "  " << color::setcolor::cyan << regions << color::setcolor::reset << '\n';
										for (const auto& it : regions)
											regionStats[it].emplace_back(cellPos);
										vec.emplace_back(std::make_pair(cellPos, std::move(regions)));
										++row_count;
									}
									else std::clog << "  " << color::setcolor::red << "No regions above threshold." << color::setcolor::reset << '\n';
								}
							}
							if (row_count == 0u && !regionStats.empty()) {
								std::clog << "Breaking early because row with index " << color::setcolor::yellow << y << color::setcolor::reset << " didn't contain anything, and it is unlikely that anything else exists." << std::endl;
								break;
							}
						}

						const auto& t_end{ CLK::now() };

						if (i == 0) throw make_exception("Failed to partition the image!");

						std::clog << "Finished processing image partitions after " << color::setcolor::green
							<< std::chrono::duration_cast<std::chrono::seconds>(std::chrono::duration<double, std::nano>(t_end - t_start))
							<< color::setcolor::reset << std::endl;
						std::clog << color::setcolor::green << vec.size() << color::setcolor::reset << " / " << color::setcolor::green << i << color::setcolor::reset << " partitions had valid color map data." << std::endl;

						vec.shrink_to_fit();

						// check if all known regions were found in the map.
						for (const auto& [color, region] : colormap) {
							if (!regionStats.contains(region))
								std::clog
								<< term::get_warn(true, 10) << "No cells found for Region:\n"
								<< indent(12) << "Editor ID:  '" << region.editorID << "'\n"
								<< indent(12) << "Map Name:   '" << region.mapName << "'\n"
								<< indent(12) << "Color:      '" << color << "'\n";
						}

						// get the target output location
						std::filesystem::path outpath{ myPath };

						if (const auto& outArg{ args.typegetv_any<opt::Flag, opt::Option>('o', "out") }; outArg.has_value())
							outpath = outArg.value(); // override the output path

						if (!std::filesystem::is_directory(outpath))
							throw make_exception("Invalid directory name: '", outpath.generic_string(), '\'');

						std::string worldspaceName{ args.typegetv_any<opt::Flag, opt::Option>('w', "worldspace").value_or("worldspace") };
						std::filesystem::path outRegionData{ outpath / (worldspaceName + ".region.txt") }, outMapData{ outpath / (worldspaceName + ".map.txt") };

						// write the output region config file
						if (ini.write(outRegionData))
							std::clog << "Successfully saved region data to '" << color::setcolor::yellow << outRegionData.generic_string() << color::setcolor::reset << '\'' << std::endl;
						else std::clog << term::get_error() << "Failed to write region data to '" << color::setcolor::yellow << outRegionData.generic_string() << color::setcolor::reset << '\'' << std::endl;
						// write the output region map file
						if (file::write(outMapData, "[RegionAreas]\n", regionStats, "\n[HoldMap]\n", vec))
							std::clog << "Successfully saved the lookup matrix to '" << color::setcolor::yellow << outMapData.generic_string() << color::setcolor::reset << '\'' << std::endl;
						else std::clog << term::get_error() << "Failed to write map data to '" << color::setcolor::yellow << outMapData.generic_string() << color::setcolor::reset << '\'' << std::endl;

						// if a window is open, close it
						if (display_each) cv::destroyWindow(windowName);
					}
					else if (args.checkopt("display")) {
						std::clog << "Opening display..." << std::endl;
						img.openDisplay();
						std::clog << "Press any key when the window is open to exit." << std::endl;
						cv::waitKey(windowTimeout);
						img.closeDisplay();
					}
					else throw make_exception("No arguments were included that specify what to do with the image! ('-d'/'--dim', '--display')");
				}
				else throw make_exception("Failed to load image file '", path, '\'');

				streams.reset(StandardStream::ALL);
			}
			else throw make_exception("Failed to resolve filepath ", path, "! (File doesn't exist)");
		}
		else throw make_exception("Nothing to do! (No filepath was specified with '-f'/'--file')");

		return 0;
	} catch (const std::exception& ex) {
		std::cerr << term::get_error() << ex.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << term::get_crit() << "An unknown exception occurred!" << std::endl;
		return 1;
	}
}
