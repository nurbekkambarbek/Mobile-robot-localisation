clear all
clc
%Load file
load mbl_Goal.txt
load mbl_Goal_cov.txt

%Assign data to variables from file
xt=mbl_Goal(:, 1);  %robot path x-axis coordinate
yt=mbl_Goal(:, 2);  %robot path y-axis coordinate
v=mbl_Goal(:, 3);   %linear velocity
w=mbl_Goal(:, 4);   %angular velocity

%Convert to meters
xt=xt/1000;
yt=yt/1000;         
v=v/1000;           
cnt=0;
%
figure
plot(v);
title('Linear velocity', 'fontsize', 14);
xlabel('Samples [ spl ]', 'fontsize', 14);
ylabel('Linear velocity [ m/spl ]', 'fontsize', 14);
legend('Linear velocity');
axis([0 200 0 0.6]);
figure
w=w*100/3.1415;     %convert angular vel given in
                    %radians to degrees
plot(w);
axis([0 200 -3 10]);
title('Angular velocity', 'fontsize', 14);
xlabel('Samples [ spl ]', 'fontsize', 14);
ylabel('Angular velocity [ rad/spl ]', 'fontsize', 14);
legend('Angular velocity');
figure
set(gca,'fontsize',18)
plot(xt, yt, '--');
hold on
for i=1:5
     B=mbl_Goal_cov((i-1)*4+1, 1:2);            %center of the ellipse
     A=mbl_Goal_cov((i-1)*4+2:(i-1)*4+4, :);    %covariance matrix
     A=A/1000000;
     B=B/1000;
     error_ellipse(A(1:2,1:2),B,0.999999);
end
axis([-2 10 -2 10]);
title('Motion-based localisation: "Drive to Goal"', 'fontsize', 14);
xlabel('x-axis [ m ]', 'fontsize', 14);
ylabel('y - axis [ m ]', 'fontsize', 14);
legend('Estimated path', 'Pose uncertainty ellipsoid', 14);


