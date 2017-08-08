function A = translateToRelativeSunDirection(A)

for i = 1 : length(A)
    % utc im Sommer ist zwei Stunden früher
    utcString = ['2016/08/14 ' num2str(A(i,4) - 2) ':' num2str(A(i,5)) ':00'];    
    
    %http://www.wieweit.net/gps_koordinaten-hohe.php
    [Az El] = SolarAzEl(utcString, 52.457259, 13.296225, 59.5);

    % this angle is the dance direction in the map reference system (North
    % to East)
    A(i, 3) = pi*Az/180 + A(i, 3);
    
    % convert to normal handedness (counterclockwise turning and 0° in the x axis)
    A(i, 3) = 2*pi - A(i, 3) + pi/2;
    
end