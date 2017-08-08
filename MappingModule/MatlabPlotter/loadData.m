function A = loadData(datafilename)
fid = fopen(datafilename, 'r');
if fid == -1
    display('error while trying to open file')
end
A = fscanf(fid, '%d,%f,%f,%d,%d,%d');
A = reshape(A, 6, length(A) / 6 )';

fclose(fid)