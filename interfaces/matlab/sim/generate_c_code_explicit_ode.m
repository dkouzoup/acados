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

function generate_c_code_explicit_ode( model )

%% import casadi
import casadi.*

if CasadiMeta.version()=='3.4.0'
	% casadi 3.4
	casadi_opts = struct('mex', false, 'casadi_int', 'int', 'casadi_real', 'double');
else
	% old casadi versions
	error('Please download and install Casadi 3.4.0 to ensure compatibility with acados')
end

%% load model
x = model.x;
u = model.u;
f_expl = model.f_expl_expr;
model_name = model.name;

%% get model dimensions
nx = length(x);
nu = length(u);

%% set up functions to be exported
Sx = SX.sym('Sx',nx,nx);
Sp = SX.sym('Sp',nx,nu);
lambdaX = SX.sym('lambdaX',nx,1);

expl_ode_fun = Function([model_name,'_expl_ode_fun'],{x,u},{f_expl});
% TODO: Polish: get rid of SX.zeros
vdeX = SX.zeros(nx,nx);
vdeX = vdeX + jtimes(f_expl,x,Sx);

vdeP = SX.zeros(nx,nu) + jacobian(f_expl,u);
vdeP = vdeP + jtimes(f_expl,x,Sp);

expl_vde_forw = Function([model_name,'_expl_vde_forw'],{x,Sx,Sp,u},{f_expl,vdeX,vdeP});

jacX = SX.zeros(nx,nx) + jacobian(f_expl,x);
expl_ode_jac = Function([model_name,'_expl_ode_jac'],{x,u},{f_expl,jacX});

adj = jtimes(f_expl,[x;u],lambdaX,true);

expl_vde_adj = Function([model_name,'_expl_vde_adj'],{x,lambdaX,u},{adj});

S_forw = vertcat(horzcat(Sx, Sp), horzcat(zeros(nu,nx), eye(nu)));
hess = S_forw.'*jtimes(adj,[x;u],S_forw);
hess2 = [];
for j = 1:nx+nu
    for i = j:nx+nu
        hess2 = [hess2; hess(i,j)];
    end
end

expl_ode_hess = Function([model_name,'_expl_ode_hess'],{x,Sx,Sp,lambdaX,u},{adj,hess2});

%% generate C code
expl_ode_fun.generate([model_name,'_expl_ode_fun'], casadi_opts);
expl_vde_forw.generate([model_name,'_expl_vde_forw'], casadi_opts);
expl_ode_jac.generate([model_name,'_expl_ode_jac'], casadi_opts);
expl_vde_adj.generate([model_name,'_expl_vde_adj'], casadi_opts);
expl_ode_hess.generate([model_name,'_expl_ode_hess'], casadi_opts);

end