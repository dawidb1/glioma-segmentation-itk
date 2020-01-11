#include <iostream> // std::cout, std::cin
#include <string> // std::string
#include <itkImage.h> // itk::Image

#include<itkImageSeriesReader.h>
#include<itkImageSeriesWriter.h>
#include<itkGDCMImageIO.h>

#include<itkGDCMSeriesFileNames.h>
#include<itkNumericSeriesFileNames.h>

typedef std::string string;
typedef signed short PixelType;
typedef itk::Image<PixelType, 3> ImageType;

typename ImageType::Pointer ReadImage(string pathToImages) {

	typedef itk::ImageSeriesReader<ImageType> ReaderTypeSeries;

	itk::GDCMSeriesFileNames::Pointer gdcmSeriesFileNames = itk::GDCMSeriesFileNames::New();
	gdcmSeriesFileNames->SetDirectory(pathToImages);
	itk::SerieUIDContainer series = gdcmSeriesFileNames->GetSeriesUIDs();
	int imagesSizeInDir = gdcmSeriesFileNames->GetFileNames(series[0]).size();

	int counter = 0;
	std::cout << "Sciezki do plikow" << std::endl;
	for (string x : gdcmSeriesFileNames->GetFileNames(series[0])) {
		std::cout << x << std::endl;
		counter++;
	}

	ReaderTypeSeries::Pointer seriesReader = ReaderTypeSeries::New();
	seriesReader->SetFileNames(gdcmSeriesFileNames->GetFileNames(series[0]));
	seriesReader->Update();

	return seriesReader->GetOutput();
}

void SaveImage(ImageType::Pointer resultImage, string pathToResults) {

	typedef itk::ImageFileWriter<ImageType>WriterType;

	WriterType::Pointer seriesWriter = WriterType::New();
	seriesWriter->SetInput(resultImage);
	seriesWriter->SetFileName(pathToResults + "/seriaWolumen.vtk");
	seriesWriter->Update();
}

int main(int argc, char *argv[]) {

	try {
		string pathToImages = "../dane";
		string pathToResults = "../wyniki/";

		ImageType::Pointer image = ReadImage(pathToImages);
		SaveImage(image, pathToResults);

	}
	catch (itk::ExceptionObject &ex) {
		ex.Print(std::cout);
	}

	std::cout << "Hit [Enter]...";
	std::cin.get();
	return EXIT_SUCCESS;
}


