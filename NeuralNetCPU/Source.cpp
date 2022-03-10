#include <PVX_NeuralNetsCPU.h>
#include <iostream>

int main() {
	using namespace PVX::DeepNeuralNets;

	PVX::DeepNeuralNets::InputLayer Input("Input", 2);
	PVX::DeepNeuralNets::NeuronLayer Layer1("Layer 1", &Input, 8);
	PVX::DeepNeuralNets::NeuronLayer Layer2("Layer 2", &Layer1, 8);
	PVX::DeepNeuralNets::NeuronLayer Layer3("Layer 3", &Layer1, 2, LayerActivation::Linear);
	PVX::DeepNeuralNets::OutputLayer Output(&Layer3, OutputType::StableSoftMax);

	auto inp = Input.MakeRawInput({
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
	});

	float outp[]{
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
	};

	Input.InputRaw(inp);
	Output.Result();
	float err = Output.Train(outp);

	for (int j = 0; j<100000 && err>1e-4; j++) {
		for (int i = 0; i<1000 && err>1e-4; i++) {
			Output.Result();
			err = err * 0.9 + 0.1 * Output.Train(outp);
		}
		printf("Error: %.5f\r", err);
		if (!(j%10))
			std::cout << "\n" << Output.Result() << "\n";
	}
	return 0;
}