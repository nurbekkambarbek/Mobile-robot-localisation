clear all
clc

%Load file
load mbl_Straight.txt
load mbl_Straight_cov.txt

%Assign data to variables from file
xt=mbl_Straight(:, 1);
yt=mbl_Straight(:, 2);
xt=xt/1000;                                         %conversion to meters
yt=yt/1000;                                         %conversion to meters        
plot(xt, yt, '-');                                  %draw the path of the robot
hold on

%Draw intermediate uncertainty ellipsoides
for i=1:5
     B=mbl_Straight_cov((i-1)*4+1, 1:2);            %center of the ellipse
     A=mbl_Straight_cov((i-1)*4+2:(i-1)*4+4, :);    %covariance matrix
     A=A/1000000;                                   %conversion to meters
     B=B/1000;                                      %conversion to meters
     error_ellipse(A(1:2,1:2),B,0.999999);          %draws uncertainty ellipsoid
end

axis([0 2.5 -0.3 0.3]);
title('Motion-based localisation: "Drive Straight"','fontsize', 13);
xlabel('x - axis [m]','fontsize', 14);
ylabel('y - axis [m]','fontsize', 14);
legend('Estimated path', 'Pose uncertainty ellipsoid');