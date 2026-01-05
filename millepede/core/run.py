import pede
import time

pede.load_sim_tracks()

# for show purpose
y_data = []
x_data = []

descriptor = "check_fix_layer_correct"
pede.NITER = 100
pede.method = "minres"

for iiter in range(0, pede.NITER):
    print(".................. iteration {} ....................".format(iiter))
    #start_time = time.time()
    chisq = pede.one_iteration(iiter*pede.nBatch, pede.nBatch)
    #end_time = time.time()
    #print("iteration used {} seconds\n".format(end_time - start_time))
    #input("enter to continue...")

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




