import matplotlib.pyplot as plt

df = pd.read_csv('data.csv')

plt.plot(df['x'], df['y'], marker='o')
plt.xlabel('Cantidad de puertas')
plt.ylabel('Tiempo (ms)')
plt.title('Coste transformación de circuito')
plt.legend()
plt.grid()

plt.savefig('grafico_basico.png', dpi=300, bbox_inches='tight')  # Ajustar resolución y bordes