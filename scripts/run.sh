../bin/Release/yimggrade ../tests/greg_zaal_artist_workshop.hdr -e 0 -o ../out/greg_zaal_artist_workshop_01.jpg
../bin/Release/yimggrade ../tests/greg_zaal_artist_workshop.hdr -e 1 -f -c 0.75 -s 0.75 -o ../out/greg_zaal_artist_workshop_02.jpg
../bin/Release/yimggrade ../tests/greg_zaal_artist_workshop.hdr -e 0.8 -c 0.6 -s 0.5 -g 0.5 -o ../out/greg_zaal_artist_workshop_03.jpg

../bin/Release/yimggrade ../tests/toa_heftiba_people.jpg -e -1 -f -c 0.75 -s 0.3 -v 0.4 -o ../out/toa_heftiba_people_01.jpg
../bin/Release/yimggrade ../tests/toa_heftiba_people.jpg -e -0.5 -c 0.75 -s 0 -o ../out/toa_heftiba_people_02.jpg
../bin/Release/yimggrade ../tests/toa_heftiba_people.jpg -e -0.5 -c 0.6 -s 0.7 -tr 0.995 -tg 0.946 -tb 0.829 -g 0.3 -o ../out/toa_heftiba_people_03.jpg
../bin/Release/yimggrade ../tests/toa_heftiba_people.jpg -m 16 -G 16 -o ../out/toa_heftiba_people_04.jpg
