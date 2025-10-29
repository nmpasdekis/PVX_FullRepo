#ifndef __PVX_AUTOMATA_H__
#define __PVX_AUTOMATA_H__

#include <initializer_list>
#include <unordered_map>
#include <vector>
#include <set>

namespace PVX {
	namespace Automata {
		typedef enum class Command { Skip, Read, Push, Pop, Error } Command;

		template<typename StateType, typename InputSymbolType = char, typename StackSymbolType = char>
		class DeterministicPushdownAutomaton {
		public:
			struct TransitionCommand {
				StateType State;
				Command Stack;
				Command Input;
				StackSymbolType StackSymbol;
			};
			class Transition {
			public:
				Transition(StateType state,
					InputSymbolType input,
					StackSymbolType stackTop,
					StateType nextState,
					Command stackCommand,
					StackSymbolType PushItem = StackSymbolType{},
					Command inputCommand = Command::Read);
				Transition(StateType state,
					StackSymbolType stackTop,
					StateType nextState,
					Command stackCommand,
					StackSymbolType PushItem = StackSymbolType{},
					Command inputCommand = Command::Read);
				Transition(StateType state,
					InputSymbolType input,
					StateType nextState,
					Command stackCommand,
					StackSymbolType PushItem = StackSymbolType{},
					Command inputCommand = Command::Read);
				Transition(StateType state,
					StateType nextState,
					Command stackCommand,
					StackSymbolType PushItem = StackSymbolType{},
					Command inputCommand = Command::Read);



				StateType State, NextState;
				InputSymbolType Input;
				StackSymbolType StackTop, PushItem;
				Command InputCommand, StackCommand;
				bool DefaultInput = false, DefaultStackTop = false;
			};

			DeterministicPushdownAutomaton(const std::initializer_list<Transition> & Symbols, const std::initializer_list<StateType> & EndState, StackSymbolType InitialStackTop);
			Command Consume(const InputSymbolType & Symbol);
			bool Consume(const InputSymbolType * Symbols, int Count);
			bool Consumed();
			void Reset();
			std::vector<StackSymbolType> & GetStack();
			int GetState() const;
		private:
			int CurrentState, StartState;
			StackSymbolType InitialTop;
			std::unordered_map<StateType, std::unordered_map<InputSymbolType, std::unordered_map<StackSymbolType, TransitionCommand>>> AutoMatrix;
			std::unordered_map<StateType, std::unordered_map<StackSymbolType, TransitionCommand>> DefaultAutoMatrix;
			std::vector<StackSymbolType> Stack;
			std::set<StateType> End;
		};

		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline PVX::Automata::DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::DeterministicPushdownAutomaton(const std::initializer_list<Transition> & Symbols, const std::initializer_list<StateType> & EndState, StackSymbolType InitialStackTop) {
			InitialTop = InitialStackTop;
			Stack.push_back(InitialTop);
			CurrentState = StartState = Symbols.begin()->State;
			for (auto & s : Symbols) {
				if(!s.Default)
					AutoMatrix[s.State][s.Input][s.StackTop] = TransitionCommand{ s.NextState, s.StackCommand, s.InputCommand, s.PushItem };
				else
					DefaultAutoMatrix[s.State][s.StackTp] = TransitionCommand{ s.NextState, s.StackCommand, s.InputCommand, s.PushItem };
			}
			for (StateType i : EndState) {
				End.insert(i);
			}
		}

		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline Command DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::Consume(const InputSymbolType & Symbol) {
			auto & State = AutoMatrix[CurrentState];
			const auto & findInput = State.find(Symbol);
			if (findInput != State.end()) {
				const auto & findStackTop = findInput->second.find(Stack.back());
				if (findStackTop != findInput->second.end()) {
					auto & cmd = findStackTop->second;
					switch (cmd.Stack) {
					case Command::Push:
						Stack.push_back(cmd.StackSymbol);
						break;
					case Command::Pop:
						if (!Stack.size())return Command::Error;
						Stack.pop_back();
						break;
					}
					return cmd.Input;
				}
			};
			return Command::Error;
		}

		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline bool DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::Consume(const InputSymbolType * Symbols, int Count) {
			Command Move = Command::Read;
			for (int i = 0; Move != Command::Error && i < Count; i += (int)Move) {
				Move = Consume(Symbols[i]);
			}
			return Move != Command::Error && Consumed();
		}

		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline bool DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::Consumed() {
			return Stack.size() == 1 && Stack.back() == InitialTop && End.find(CurrentState) != End.end();
		}

		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline void DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::Reset() {
			Stack.clear();
			CurrentState = StartState;
		}

		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline std::vector<StackSymbolType>& DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::GetStack() {
			return Stack;
		}

		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline int DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::GetState() const {
			return CurrentState;
		}

		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::Transition::Transition(
			StateType state, 
			InputSymbolType input, 
			StackSymbolType stackTop, 
			StateType nextState, 
			Command stackCommand, 
			StackSymbolType pushItem, 
			Command inputCommand) :
				State(state),
				Input(input),
				StackTop(stackTop),
				NextState(nextState),
				InputCommand(inputCommand),
				StackCommand(stackCommand),
				PushItem(pushItem) {}
		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::Transition::Transition(
			StateType state,
			StackSymbolType stackTop,
			StateType nextState,
			Command stackCommand,
			StackSymbolType pushItem,
			Command inputCommand):
				State(state),
				Input(InputSymbolType{}),
				StackTop(stackTop),
				NextState(nextState),
				InputCommand(inputCommand),
				StackCommand(stackCommand),
				PushItem(pushItem),
				DefaultInput{ true } {}
		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::Transition::Transition(
			StateType state,
			InputSymbolType input,
			StateType nextState,
			Command stackCommand,
			StackSymbolType pushItem,
			Command inputCommand):
				State(state),
				Input(input),
				StackTop(StackSymbolType{}),
				NextState(nextState),
				InputCommand(inputCommand),
				StackCommand(stackCommand),
				PushItem(pushItem),
				DefaultStackTop{ true }	{}
		template<typename StateType, typename InputSymbolType, typename StackSymbolType>
		inline DeterministicPushdownAutomaton<StateType, InputSymbolType, StackSymbolType>::Transition::Transition(
			StateType state,
			StateType nextState,
			Command stackCommand,
			StackSymbolType pushItem,
			Command inputCommand):
				State(state),
				Input(InputSymbolType{}),
				StackTop(StackSymbolType{}),
				NextState(nextState),
				InputCommand(inputCommand),
				StackCommand(stackCommand),
				PushItem(pushItem),
				DefaultInput{ true }, DefaultStackTop{ true } {}
	}
}
#endif