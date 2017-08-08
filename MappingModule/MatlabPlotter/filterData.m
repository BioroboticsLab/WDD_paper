% discards to short waggles (shorter than 200 ms)
% clusters waggle runs and discards short dances ( < 4 waggle runs)
% return matrix of per-dance-averaged waggle runs
function B = filterData(A)
verbose = 0;
% throw away short waggles
A( find( A(:, 1) < 200 ),:) = [];   

size(A)

%cluster data based on x,y and timestamp
Q       = A(:, 3:5);
% cluster IDs (rows in ClID corresponds to rows in A (or Q)) 
ClID   = clusterdata(Q, 1);
% sort cluster IDs I are respective indices in A (or Q)
[~, I] = sort(ClID);

% find indices where cluster ID changes
idx_end_of_cluster      = [ find( diff( ClID(I) ) > 0 )];
idx_start_of_cluster    = [ 1; idx_end_of_cluster+1];
idx_end_of_cluster      = [ idx_end_of_cluster; length(I)];

B = []
d = 0; % counts discarded waggle runs
a = 0; % counts accepted waggle runs

for i = 1 : length(idx_start_of_cluster)
    idx     = idx_start_of_cluster(i) : idx_end_of_cluster(i);
    L       = length(idx);
    if verbose
        fprintf( 'cluster ID: %d \t\tsize:%d', ClID(I(idx(1))), L);
    end
    if L < 4
        if verbose 
            fprintf( ' DISCARDED \n' );
        end
        d = d + 1;
    else
        if verbose
            fprintf( ' GOOD \n' );
            A( I(idx), 1:4)
        end
        a = a + L;
        
        % is circMean working correctly?
        B = [B; mean(A( I(idx), 1)) circMean(A( I(idx), 2)) A( I(idx(1)), 6:7) L];
        
    end   
    
end

fprintf('%d dances discarded \n %d dances accepted  \n%.2f average waggle runs per dance in accepted list\n', d, size(B, 1), a / size(B,1));  