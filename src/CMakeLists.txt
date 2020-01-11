cmake_minimum_required(VERSION 2.8)

PROJECT(ITK_Glioblastoma) # nazwa solucji

FIND_PACKAGE(ITK REQUIRED) # wskazanie dla CMake, aby znalazl informacje o ITK
IF( ITK_FOUND )
	INCLUDE (${ITK_USE_FILE}) # oraz dopisal je do ustawien naszego projektu
ELSE( ITK_FOUND )
	MESSAGE(FATAL_ERROR "Nie znaleziono ITK! Ustaw ITK_DIR np. do c:\Insight\builds\ITK481.x64")
ENDIF( ITK_FOUND )

ADD_EXECUTABLE( GlioblastomaSegmentation # nazwa projektu oraz wynikowego pliku .exe
	main.cxx # plik z implementacj¹ main()
		# oraz inne pliki majace pojawic sie w drzewku projektu w VS
)

TARGET_LINK_LIBRARIES( GlioblastomaSegmentation # nazwa projektu, taka jak w ADD_EXECUTABLE(...)
	${ITK_LIBRARIES} # pod ta zmienna znajduja sie wszystkie konieczne dla linkera biblioteki
)