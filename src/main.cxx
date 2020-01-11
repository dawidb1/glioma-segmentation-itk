#include <iostream> // std::cout, std::cin
#include <string> // std::string
#include <itkImage.h> // itk::Image


#include<itkImageSeriesReader.h>
#include<itkImageSeriesWriter.h>
#include<itkGDCMImageIO.h>
#include<itkGDCMSeriesFileNames.h>
#include<itkNumericSeriesFileNames.h>

int main() {

	typedef std::string string;
	typedef signed short PixelType;
	typedef itk::Image<PixelType, 3> ImageType;
	typedef itk::ImageFileReader<ImageType>ReaderType;
	typedef itk::ImageFileWriter<ImageType>WriterType;
	typedef itk::ImageSeriesReader<ImageType> ReaderTypeSeries;
	typedef itk::Image<short, 3> ImgType3D;
	typedef itk::Image<short, 2> ImgType2D;
	typedef itk::ImageSeriesWriter<ImgType3D, ImgType2D> WriterTypeSeries;

	try {
		string pathToImages = "../dane";
		string pathToResults = "../wyniki/";

		itk::GDCMSeriesFileNames::Pointer gdcmSeriesFileNames = itk::GDCMSeriesFileNames::New();
		gdcmSeriesFileNames->SetDirectory(pathToImages);
		itk::SerieUIDContainer series = gdcmSeriesFileNames->GetSeriesUIDs();
		int imagesSizeInDir = gdcmSeriesFileNames->GetFileNames(series[0]).size();

		for (int i = 0; i < series.size(); i++) {
			std::cout << "UID serii" << std::endl;
			std::cout << series[i] << std::endl;
		}

		int counter = 0;
		std::cout << "sciezki do plikow" << std::endl;
		for (string x : gdcmSeriesFileNames->GetFileNames(series[0])) {
			std::cout << x << std::endl;

			//odczyt i zapis pojedynczego pliku
			ReaderType::Pointer reader = ReaderType::New();
			reader->SetFileName(x);
			reader->Update();
			ImageType::Pointer image = reader->GetOutput();

			WriterType::Pointer writer = WriterType::New();
			string fileResultName = pathToResults + std::to_string(counter);

			writer->SetFileName(fileResultName + ".dcm");
			writer->SetInput(image);
			writer->Update();

			writer->SetFileName(fileResultName + ".vtk");
			writer->Update();

			counter++;
		}

		//odczyt i zaspi jako obraz wolumeryczny
		ReaderTypeSeries::Pointer seriesReader = ReaderTypeSeries::New();
		seriesReader->SetFileNames(gdcmSeriesFileNames->GetFileNames(series[0]));
		seriesReader->Update();

		ImageType::Pointer resultImage = seriesReader->GetOutput();

		WriterType::Pointer seriesWriter = WriterType::New();
		seriesWriter->SetInput(resultImage);
		seriesWriter->SetFileName(pathToResults+ "/seriaWolumen.vtk");
		seriesWriter->Update();

		//Zapis pojedynczych obrazów z serii obrazów
		ImgType3D::Pointer image = seriesReader->GetOutput();

		itk::NumericSeriesFileNames::Pointer namesGenerator2;
		namesGenerator2 = itk::NumericSeriesFileNames::New();
		namesGenerator2->SetSeriesFormat(pathToResults+ "/IMG\%05d.dcm");
		namesGenerator2->SetStartIndex(1);
		namesGenerator2->SetEndIndex(image->GetLargestPossibleRegion().GetSize()[2]);

		WriterTypeSeries::Pointer writerSeries = WriterTypeSeries::New();
		writerSeries->SetFileNames(namesGenerator2->GetFileNames());
		writerSeries->SetInput(image);
		writerSeries->Update();

	}
	catch (itk::ExceptionObject &ex) {
		ex.Print(std::cout);
	}
	catch (std::exception &ex) {
		std::cout << ex.what() << std::endl;
	}
	catch (...) {
		std::cout << "NFI what happened!" << std::endl;
	}
	std::cout << "Hit [Enter]...";
	std::cin.get();
	return EXIT_SUCCESS;
}