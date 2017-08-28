clear all
clc

%Load file
load ekf.txt
load ekf_cov.txt

%Assign data to variables from file
xt=ekf(:, 1);
yt=ekf(:, 2);

A=ekf_cov(:, :);            %covariance matrix
B=ekf(end, :);              %center of the ellipse

%Convert to meters
xt=xt/1000;                 
A=A/1000;
yt=yt/1000; 
B=B/1000;

plot(xt, yt, '--');         %path of the robot
hold on
error_ellipse(A(1:2,1:2),B,0.99);   %uncertainty ellipsoid
axis([0 2.5 -0.1 0.1]);

title('Kalman filter localisation - "Drive Straight"','fontsize', 14);
xlabel('x - axis [ m ]','fontsize', 14);
ylabel('y - axis [ m ]','fontsize', 14);
legend('Estimated path', 'Pose uncertainty ellipsoid');


