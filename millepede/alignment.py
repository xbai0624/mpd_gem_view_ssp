from core import pede
from core.text_parser import vec3_t

def load_tracks(path = """../log_tracks.txt"""):
    res = []
    with open(path) as f:
        lines = f.readlines()
        for l in lines[1:]:
            elements = l.split()
            t = [vec3_t(float(elements[4*k+1]), float(elements[4*k+2]), float(elements[4*k+3])) for k in range(int(len(elements)/4))]
            t[1], t[2] = t[2], t[1]
            res.append(t)
    return res;

pede.N_on_T = 3
pede.nBatch = 40
pede.NITER = 30
pede.total_tracks = load_tracks()
pede.method = "gmres"

print("total tracks = {}".format(len(pede.total_tracks)))

x_data = []
y_data = []

for iiter in range(0, pede.NITER):
    print("....................... iteratioin {} ...................".format(iiter))
    chisq = pede.one_iteration(0, pede.nBatch)

    pede.print_tensor(pede.global_params)
    print("chi square = ", chisq)
    y_data.append(chisq)
    x_data.append(iiter)



#................................. program ends here..........................................
# for show purpose
import matplotlib.pyplot as plt
plt.ion() # enable interactive mode
fig, ax = plt.subplots()
line, = ax.plot(x_data, y_data, 'b-')

'''
# save chi2 to a text file
if pede.USE_MOMENTUM:
    file_name = "chi2_step_size_{}_momentum_eta_{}_regularization_lambda_{}_{}_{}.txt".format(pede.step_size, pede.eta, pede.regularization_lambda, pede.method, descriptor)
else:
    file_name = "chi2_step_size_{}_no_momentum_regularization_lambda_{}_{}_{}.txt".format(pede.step_size, pede.eta, pede.regularization_lambda, pede.method, descriptor)
with open(file_name, 'w') as file:
    file.write("step_size = {}, momentum_eta = {}, regularization_lambda = {}, method = {}\n".format(pede.step_size, pede.eta, pede.regularization_lambda, pede.method))
    formated_str = '  '.join(f"{number:.4f}" for number in y_data)
    file.write(formated_str)
'''

#line.set_xdata(x_data)
#line.set_ydata(y_data)
ax.relim()
ax.autoscale_view()
fig.canvas.draw()
fig.canvas.flush_events()

# keep the plot after the loop ends
plt.ioff()
plt.show()




