//#include "Node.h"

/*
 * Node::Node(IUnknown* pE, IDispatch* pD) : pEntity(pE), pDefinition(pD) {
 * 	if (pEntity) pEntity->AddRef();
 * 	if (pDefinition) pDefinition->AddRef();
 * 	if (pEntity && !pDefinition) {
 * 		K5::ksEntityPtr entity = pEntity;
 * 		IDispatchPtr def = entity->GetDefinition();
 * 		def.AddRef();
 * 		pDefinition = def.GetInterfacePtr();
 * 	}
 * }
 * 
 * Node::~Node() {
 * 	if (pDefinition) pDefinition->Release();
 * 	if (pEntity) pEntity->Release();
 * }
 */

/*
 * int Node::GetType() const {
 * 	K5::ksEntityPtr entity = pEntity;
 * 	return entity ? entity->type : 0;
 * }
 * 
 * std::string Node::GetName() const {
 * 	K5::ksEntityPtr entity = pEntity;
 * 	if (!entity) return std::string();
 * 	return Cp1251ToUtf8(entity->name);
 * }
 * 
 * Node& Node::SetName(const std::string& name) {
 * 	K5::ksEntityPtr entity = pEntity;
 * 	if (entity) {
 * 		entity->name = Utf8ToCp1251(name).c_str();
 * 	}
 * 	return *this;
 * }
 * 
 * Node& Node::Update() {
 * 	K5::ksEntityPtr entity = pEntity;
 * 	if (entity) entity->Update();
 * 	return *this;
 * }
 * 
 * Node& Node::operator=(const Node& other) {
 * 	if (this == &other) return *this;
 * 	if (pDefinition) pDefinition->Release();
 * 	if (pEntity) pEntity->Release();
 * 
 * 	pEntity = other.pEntity;
 * 	if (pEntity) pEntity->AddRef();
 * 	pDefinition = other.pDefinition;
 * 	if (pDefinition) pDefinition->AddRef();
 * 	if (pEntity && !pDefinition) {
 * 		K5::ksEntityPtr entity = pEntity;
 * 		IDispatchPtr def = entity->GetDefinition();
 * 		def.AddRef();
 * 		pDefinition = def.GetInterfacePtr();
 * 	}
 * 	return *this;
 * }
 * 
 * int Node::TYPE = KConst3D::o3d_unknown;
 */
