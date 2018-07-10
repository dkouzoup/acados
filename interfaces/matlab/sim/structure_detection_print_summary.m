function structure_detection_print_summary(gnsf, initial_model, reordered_model)
%
%   This file is part of acados.
%
%   acados is free software; you can redistribute it and/or
%   modify it under the terms of the GNU Lesser General Public
%   License as published by the Free Software Foundation; either
%   version 3 of the License, or (at your option) any later version.
%
%   acados is distributed in the hope that it will be useful,
%   but WITHOUT ANY WARRANTY; without even the implied warranty of
%   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
%   Lesser General Public License for more details.
%
%   You should have received a copy of the GNU Lesser General Public
%   License along with acados; if not, write to the Free Software Foundation,
%   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
%
%   Author: Jonathan Frey: jonathanpaulfrey(at)gmail.com

%% Description
% this function prints the most important info after determining a GNSF
% reformulation of the implicit model "initial_model" into "gnsf", which is
% equivalent to the "reordered_model".

% % GNSF
% get dimensions
nx  = gnsf.nx;
nu  = gnsf.nu;
nz  = gnsf.nz;

nx1 = gnsf.nx1;
nx2 = gnsf.nx2;
n_out = gnsf.n_out;
ny = gnsf.ny;
nuhat = gnsf.nuhat;

%
n_nodes_initial = initial_model.f_impl_expr.n_nodes();
x_old = initial_model.x;
f_impl_old = initial_model.f_impl_expr;


x = reordered_model.x;
f_impl_expr = reordered_model.f_impl_expr;

phi_current = gnsf.phi_expr;

%% PRINT SUMMARY -- STRUCHTRE DETECTION
disp(' ');
disp(['========================= STRUCTURE DETECTION SUMMARY ====================================']);
disp(' ');
disp(' successfully transcribed implicit ODE/ index-1 DAE into GNSF structure ');
disp(' ');
disp(['recuced dimension of nonlinearity phi from        ', sprintf('%6s', num2str(nx+nz)),      ' to ', sprintf('%6s', num2str(gnsf.n_out))]);
disp(['recuced input dimension of nonlinearity phi from  ', sprintf('%6s', num2str(2*nx+nz+nu)), ' to ', sprintf('%6s', num2str(gnsf.ny + gnsf.nuhat))]);
if gnsf.nx2 >0
    disp(' ');
    disp(['introduced Linear Output System of size           ', sprintf('%6s', num2str(gnsf.nx2)),'']);
    disp('consisting of the states:');
    disp(' ');
    disp(x(gnsf.nx1+1:gnsf.nx));
    disp(' ');
end

disp(['recuced number of nodes in CasADi expression of']);
disp(['nonlinearity phi from                             '...
    , sprintf('%6s', num2str(n_nodes_initial)), ' to ', sprintf('%6s', num2str(phi_current.n_nodes()))]);

compare_x = (x_old == x);
if ~compare_x.is_constant()
disp(' ');
disp('--------------------------------------------------------------------------------------------------');
disp('NOTE: permuted state vector x, such that the implicit model, can take it in the same order as GNSF');
disp(' ');
disp(' OLD / initial state vector read as: ');
disp(x_old);
disp(' ');
disp(' whereas NEW / permuted state vector reads as: ');
disp(x);
end

compare_f = (f_impl_old == f_impl_expr);
if ~compare_f.is_constant()
disp(' ');
disp('--------------------------------------------------------------------------------------------------');
disp('NOTE: permuted implicit function, such that the implicit & GNSF model, have the same order');
disp(' ');
disp(' OLD / initial state f_impl read as: ');
disp(' ');
print_casadi_function(f_impl_old);
disp(' ');
disp(' whereas NEW / permuted f_impl reads as: ');
disp(' ');
print_casadi_function(f_impl_expr);
end

if ~ isempty(reordered_model.equ_changed_sign)
    disp(' ');
    disp('--------------------------------------------------------------------------------------------------------');
    disp(' ');
    disp('During the reformulation we changed the signs (multiplied with -1) of the Equations with initial index');
    disp(' ');
    disp(reordered_model.equ_changed_sign);
end

%% print GNSF dimenstions
disp('--------------------------------------------------------------------------------------------------------');
disp(' ');
disp('The dimensions of the GNSF reformulated model read as:');
T_dim = table(nx, nu, nz, nx1, nx2, n_out, ny, nuhat);
disp( T_dim )

end


%% auxilary function
function print_casadi_function(f)


for ii = 1:length(f)
    disp(f(ii));
end

end