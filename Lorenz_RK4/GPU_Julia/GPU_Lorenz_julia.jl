using DifferentialEquations, DiffEqGPU, CUDAnative, CUDAdrv, CPUTime, Statistics

#settings
const numberOfParameters = 46080
const rollOut = 2
const numberOfRuns = 3
const gpuID = 0 #Nvidia titan black device

#select device
device!(CuDevice(gpuID))
println("Running on "*string(CuDevice(gpuID)))

#ode
function lorenz!(du,u,p,t)
  @inbounds begin
    for j in 1:rollOut
      index = 3(j-1)+1
      du[index] = 10.0 * (u[index+1] - u[index])
      du[index+1] = p[j] * u[index] - u[index+1] - u[index] * u[index+2]
      du[index+2] = u[index] * u[index+1] - 2.66666666 * u[index+2]
    end
  end
  nothing
end

#parameters and initial conditions
parameterList = range(0.0,stop = 21.0,length=numberOfParameters)
p = zeros(rollOut,1)
tspan = (0.0,10.0)
u0 = ones(rollOut*3,1)

#parameter change function
#parameterChange = (prob,i,repeat) -> remake(prob,p=parameterList[i]) #it has slightly more allocations
function parameterChange!(prob,i,repeat)
  index = Int64(rollOut*i)+1
  for j in 1:rollOut
    prob.p[j] = parameterList[index-rollOut]
  end
  prob
end

#defining ode problem
lorenzProblem =  ODEProblem(lorenz!,u0,tspan,p)
ensembleProb = EnsembleProblem(lorenzProblem,prob_func=parameterChange!)

#simulation
times = Vector{Float64}(undef,numberOfRuns)
for runs in 1:numberOfRuns
  tStart = CPUtime_us()
  solve(
    ensembleProb,
    RK4(),
    EnsembleGPUArray(),
    trajectories=Int64(numberOfParameters/rollOut),
    save_everystep = false,
    save_start = false,
    save_end = true,
    adaptive = false,
    dt = 0.01,
    dense = false
    )
  tEnd = CPUtime_us()
  times[runs] = (tEnd-tStart)/(10^6)
end

println("end")
println("Parameter number: "*string(numberOfParameters))
println("Time: "*string(times))
